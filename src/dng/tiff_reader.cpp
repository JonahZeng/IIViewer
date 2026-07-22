#include "dng/tiff_reader.h"

#include <algorithm>
#include <cstring>

namespace dng {

TiffReader::TiffReader(const std::string& path)
    : little_endian_(true) {
    file_.open(path, std::ios::binary);
    if (!file_.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    // Read byte order mark (first 2 bytes)
    uint8_t bom[2];
    file_.read(reinterpret_cast<char*>(bom), 2);
    if (bom[0] == 'I' && bom[1] == 'I') {
        little_endian_ = true;
    } else if (bom[0] == 'M' && bom[1] == 'M') {
        little_endian_ = false;
    } else {
        throw std::runtime_error("Invalid TIFF byte order mark");
    }

    // Read magic number 42 (0x002A)
    uint16_t magic = read_u16(2);
    if (magic != 42) {
        throw std::runtime_error("Invalid TIFF magic number: " + std::to_string(magic));
    }
}

TiffReader::~TiffReader() {
    if (file_.is_open()) {
        file_.close();
    }
}

void TiffReader::read_at(uint64_t offset, void* buf, size_t len) {
    file_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    file_.read(static_cast<char*>(buf), static_cast<std::streamsize>(len));
    if (!file_) {
        throw std::runtime_error("Read failed at offset " + std::to_string(offset)
                                 + " len " + std::to_string(len));
    }
}

// --- Byte swap helpers ---

template<typename T>
T TiffReader::byte_swap(T val) {
    if (little_endian_) {
        return val;  // host is little-endian on x86/ARM
    }
    // Big-endian file: swap bytes
    T result;
    auto* src = reinterpret_cast<uint8_t*>(&val);
    auto* dst = reinterpret_cast<uint8_t*>(&result);
    for (size_t i = 0; i < sizeof(T); ++i) {
        dst[i] = src[sizeof(T) - 1 - i];
    }
    return result;
}

template<typename T>
T TiffReader::read_scalar(uint64_t offset) {
    T val;
    read_at(offset, &val, sizeof(T));
    return byte_swap(val);
}

uint16_t TiffReader::read_u16(uint64_t offset) {
    return read_scalar<uint16_t>(offset);
}

uint32_t TiffReader::read_u32(uint64_t offset) {
    return read_scalar<uint32_t>(offset);
}

int32_t TiffReader::read_s32(uint64_t offset) {
    return read_scalar<int32_t>(offset);
}

float TiffReader::read_f32(uint64_t offset) {
    return read_scalar<float>(offset);
}

double TiffReader::read_f64(uint64_t offset) {
    return read_scalar<double>(offset);
}

// --- IFD parsing ---

IFD TiffReader::parse_ifd(uint64_t offset) {
    IFD ifd;
    ifd.offset = offset;

    uint16_t num_entries = read_u16(offset);
    ifd.tags.reserve(num_entries);

    uint64_t cursor = offset + 2;
    for (uint16_t i = 0; i < num_entries; ++i) {
        TiffTag tag;
        tag.entry_offset = cursor;
        tag.tag_id = read_u16(cursor);
        tag.type = static_cast<TiffType>(read_u16(cursor + 2));
        tag.count = read_u32(cursor + 4);

        // Read the 4-byte value/offset field
        uint32_t raw_val = read_u32(cursor + 8);
        tag.value_or_offset = raw_val;

        ifd.tags.push_back(tag);
        cursor += 12;
    }

    ifd.next_ifd_offset = read_u32(cursor);
    return ifd;
}

const TiffTag* TiffReader::find_tag(const IFD& ifd, uint16_t tag_id) const {
    for (const auto& tag : ifd.tags) {
        if (tag.tag_id == tag_id) {
            return &tag;
        }
    }
    return nullptr;
}

// --- Tag data offset computation ---

uint64_t TiffReader::tag_data_offset(const TiffTag& tag) const {
    uint32_t total_bytes = tiff_type_size(tag.type) * tag.count;
    if (total_bytes <= 4) {
        // Value is inline in the 4-byte field
        // For little-endian files, the value starts at byte 0 of the field.
        // For big-endian files, the value is at the beginning of the 4-byte field too.
        // The value_or_offset field was already byte-swapped as a uint32,
        // but for inline data we need to re-interpret the raw bytes.
        // We return the entry_offset + 8 (position of the value field).
        return tag.entry_offset + 8;
    }
    // Data is at the offset stored in value_or_offset
    return tag.value_or_offset;
}

// --- Typed accessors ---

uint16_t TiffReader::get_u16(const TiffTag& tag) {
    if (tag.count < 1) throw std::runtime_error("Empty tag");
    uint64_t off = tag_data_offset(tag);
    return read_u16(off);
}

uint32_t TiffReader::get_u32(const TiffTag& tag) {
    if (tag.count < 1) throw std::runtime_error("Empty tag");
    uint64_t off = tag_data_offset(tag);
    return read_u32(off);
}

float TiffReader::get_f32(const TiffTag& tag) {
    if (tag.count < 1) throw std::runtime_error("Empty tag");
    uint64_t off = tag_data_offset(tag);
    return read_f32(off);
}

double TiffReader::get_f64(const TiffTag& tag) {
    if (tag.count < 1) throw std::runtime_error("Empty tag");
    uint64_t off = tag_data_offset(tag);
    return read_f64(off);
}

std::vector<uint8_t> TiffReader::get_bytes(const TiffTag& tag) {
    uint64_t off = tag_data_offset(tag);
    std::vector<uint8_t> buf(tag.count);
    if (tag.count > 0) {
        read_at(off, buf.data(), tag.count);
    }
    return buf;
}

std::vector<uint16_t> TiffReader::get_u16_array(const TiffTag& tag) {
    uint64_t off = tag_data_offset(tag);
    std::vector<uint16_t> buf(tag.count);
    for (uint32_t i = 0; i < tag.count; ++i) {
        buf[i] = read_u16(off + i * 2);
    }
    return buf;
}

std::vector<uint32_t> TiffReader::get_u32_array(const TiffTag& tag) {
    uint64_t off = tag_data_offset(tag);
    std::vector<uint32_t> buf(tag.count);
    for (uint32_t i = 0; i < tag.count; ++i) {
        buf[i] = read_u32(off + i * 4);
    }
    return buf;
}

std::vector<float> TiffReader::get_f32_array(const TiffTag& tag) {
    uint64_t off = tag_data_offset(tag);
    std::vector<float> buf(tag.count);
    for (uint32_t i = 0; i < tag.count; ++i) {
        buf[i] = read_f32(off + i * 4);
    }
    return buf;
}

std::vector<double> TiffReader::get_f64_array(const TiffTag& tag) {
    uint64_t off = tag_data_offset(tag);
    std::vector<double> buf(tag.count);
    for (uint32_t i = 0; i < tag.count; ++i) {
        buf[i] = read_f64(off + i * 8);
    }
    return buf;
}

double TiffReader::get_rational(const TiffTag& tag) {
    if (tag.count < 1) throw std::runtime_error("Empty rational tag");
    uint64_t off = tag_data_offset(tag);
    uint32_t num = read_u32(off);
    uint32_t den = read_u32(off + 4);
    if (den == 0) return 0.0;
    return static_cast<double>(num) / static_cast<double>(den);
}

std::vector<double> TiffReader::get_rational_array(const TiffTag& tag) {
    uint64_t off = tag_data_offset(tag);
    std::vector<double> buf(tag.count);
    for (uint32_t i = 0; i < tag.count; ++i) {
        uint32_t num = read_u32(off + i * 8);
        uint32_t den = read_u32(off + i * 8 + 4);
        buf[i] = (den == 0) ? 0.0 : static_cast<double>(num) / static_cast<double>(den);
    }
    return buf;
}

std::string TiffReader::get_ascii(const TiffTag& tag) {
    uint64_t off = tag_data_offset(tag);
    std::vector<char> buf(tag.count);
    if (tag.count > 0) {
        read_at(off, buf.data(), tag.count);
    }
    // Ensure null termination
    if (!buf.empty() && buf.back() != '\0') {
        buf.push_back('\0');
    }
    return std::string(buf.data());
}

std::vector<uint8_t> TiffReader::get_undefined(const TiffTag& tag) {
    return get_bytes(tag);
}

double TiffReader::get_srational(const TiffTag& tag) {
    if (tag.count < 1) throw std::runtime_error("Empty srational tag");
    uint64_t off = tag_data_offset(tag);
    int32_t num = read_s32(off);
    int32_t den = read_s32(off + 4);
    if (den == 0) return 0.0;
    return static_cast<double>(num) / static_cast<double>(den);
}

std::vector<double> TiffReader::get_srational_array(const TiffTag& tag) {
    uint64_t off = tag_data_offset(tag);
    std::vector<double> buf(tag.count);
    for (uint32_t i = 0; i < tag.count; ++i) {
        int32_t num = read_s32(off + i * 8);
        int32_t den = read_s32(off + i * 8 + 4);
        buf[i] = (den == 0) ? 0.0 : static_cast<double>(num) / static_cast<double>(den);
    }
    return buf;
}

} // namespace dng

