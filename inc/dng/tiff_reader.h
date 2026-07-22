#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace dng {

// TIFF data types (TIFF6 spec, p. 19)
enum class TiffType : uint16_t {
    BYTE      = 1,   // 8-bit unsigned
    ASCII     = 2,   // 8-bit with null terminator
    SHORT     = 3,   // 16-bit unsigned
    LONG      = 4,   // 32-bit unsigned
    RATIONAL  = 5,   // two LONGs: numerator/denominator
    SBYTE     = 6,
    UNDEFINED = 7,   // 8-bit untyped
    SSHORT    = 8,
    SLONG     = 9,
    SRATIONAL = 10,
    FLOAT     = 11,
    DOUBLE    = 12,
};

inline uint32_t tiff_type_size(TiffType t) {
    switch (t) {
        case TiffType::BYTE:
        case TiffType::ASCII:
        case TiffType::SBYTE:
        case TiffType::UNDEFINED: return 1;
        case TiffType::SHORT:
        case TiffType::SSHORT:    return 2;
        case TiffType::LONG:
        case TiffType::SLONG:
        case TiffType::FLOAT:     return 4;
        case TiffType::RATIONAL:
        case TiffType::SRATIONAL:
        case TiffType::DOUBLE:    return 8;
    }
    return 0;
}

// A single TIFF IFD entry (12 bytes on disk, parsed into this struct)
struct TiffTag {
    uint16_t tag_id;
    TiffType type;
    uint32_t count;
    // Value or offset. If total bytes <= 4, value is stored inline
    // in these 4 bytes. Otherwise it's a file offset to the data.
    uint32_t value_or_offset;
    // Byte offset of this tag's 12-byte entry in the file (for debugging)
    uint64_t entry_offset;
};

// One IFD (Image File Directory)
struct IFD {
    uint64_t offset;
    std::vector<TiffTag> tags;
    uint64_t next_ifd_offset;
};

// Lightweight reader for TIFF/DNG container structure.
// Parses byte order, IFD chains, and SubIFDs, and provides
// typed access to tag values.
class TiffReader {
public:
    explicit TiffReader(const std::string& path);
    ~TiffReader();

    // Non-copyable, owns the file handle
    TiffReader(const TiffReader&) = delete;
    TiffReader& operator=(const TiffReader&) = delete;

    bool is_little_endian() const { return little_endian_; }

    // Read raw bytes from file at given offset
    void read_at(uint64_t offset, void* buf, size_t len);

    // Read a scalar with byte-order conversion
    uint16_t read_u16(uint64_t offset);
    uint32_t read_u32(uint64_t offset);
    int32_t  read_s32(uint64_t offset);
    float    read_f32(uint64_t offset);
    double   read_f64(uint64_t offset);

    // Parse an IFD at the given offset
    IFD parse_ifd(uint64_t offset);

    // --- Tag value accessors ---
    // Returns pointer to tag or nullptr if not found
    const TiffTag* find_tag(const IFD& ifd, uint16_t tag_id) const;

    // Typed value extraction. For inline values (<=4 bytes), reads from
    // value_or_offset field. For larger values, seeks to the offset.
    // All methods honor the file's byte order.

    uint16_t get_u16(const TiffTag& tag);
    uint32_t get_u32(const TiffTag& tag);
    float    get_f32(const TiffTag& tag);
    double   get_f64(const TiffTag& tag);

    // For arrays
    std::vector<uint8_t>  get_bytes(const TiffTag& tag);
    std::vector<uint16_t> get_u16_array(const TiffTag& tag);
    std::vector<uint32_t> get_u32_array(const TiffTag& tag);
    std::vector<float>    get_f32_array(const TiffTag& tag);
    std::vector<double>   get_f64_array(const TiffTag& tag);

    // RATIONAL = numerator/denominator, returned as double
    double get_rational(const TiffTag& tag);
    std::vector<double> get_rational_array(const TiffTag& tag);
    double get_srational(const TiffTag& tag);
    std::vector<double> get_srational_array(const TiffTag& tag);

    // ASCII string (null-terminated within the tag's data)
    std::string get_ascii(const TiffTag& tag);

    // Raw UNDEFINED bytes
    std::vector<uint8_t> get_undefined(const TiffTag& tag);

private:
    std::ifstream file_;
    bool little_endian_;

    template<typename T>
    T read_scalar(uint64_t offset);

    template<typename T>
    T byte_swap(T val);

    // Compute the effective data offset for a tag
    uint64_t tag_data_offset(const TiffTag& tag) const;
};

} // namespace dng
