#include "dng/ljpeg_decoder.h"

#include <cstring>
#include <string>

namespace dng {

// ---------------------------------------------------------------------------
// BitReader — 64-bit buffer, MSB-first, JPEG 0xFF00 unstuffing
// ---------------------------------------------------------------------------

LjpegDecoder::BitReader::BitReader(const uint8_t* data, size_t size)
    : data_(data), size_(size), pos_(0), buf_(0), bits_in_buf_(0),
      marker_found_(false) {}

void LjpegDecoder::BitReader::fill() {
    while (bits_in_buf_ <= 56 && pos_ < size_ && !marker_found_) {
        uint8_t byte = data_[pos_++];
        if (byte == 0xFF) {
            if (pos_ < size_) {
                uint8_t next = data_[pos_];
                if (next == 0x00) {
                    pos_++;
                    // Stuffed byte — 0xFF is data, fall through
                } else if (next >= 0xD0 && next <= 0xD7) {
                    pos_++;
                    marker_found_ = true;
                    continue;
                } else {
                    marker_found_ = true;
                    continue;
                }
            } else {
                marker_found_ = true;
                continue;
            }
        }
        buf_ = (buf_ << 8) | byte;
        bits_in_buf_ += 8;
    }
}

int LjpegDecoder::BitReader::get_bit() {
    if (bits_in_buf_ == 0) fill();
    if (bits_in_buf_ == 0) return 0;
    return static_cast<int>((buf_ >> --bits_in_buf_) & 1);
}

int LjpegDecoder::BitReader::get_bits(int n) {
    while (bits_in_buf_ < n) {
        int prev = bits_in_buf_;
        fill();
        if (bits_in_buf_ == prev) break;
    }
    if (bits_in_buf_ < n) return 0;
    bits_in_buf_ -= n;
    return static_cast<int>((buf_ >> bits_in_buf_) & ((1ull << n) - 1));
}

int LjpegDecoder::BitReader::peek_bits(int n) {
    while (bits_in_buf_ < n) {
        int prev = bits_in_buf_;
        fill();
        if (bits_in_buf_ == prev) break;
    }
    if (bits_in_buf_ < n) {
        // Return what we have, left-aligned in the low bits
        int avail = bits_in_buf_;
        if (avail == 0) return 0;
        return static_cast<int>((buf_ >> 0) & ((1ull << n) - 1));
    }
    return static_cast<int>((buf_ >> (bits_in_buf_ - n)) & ((1ull << n) - 1));
}

void LjpegDecoder::BitReader::consume(int n) {
    bits_in_buf_ -= n;
    if (bits_in_buf_ < 0) bits_in_buf_ = 0;
}

// ---------------------------------------------------------------------------
// Huffman lookup table — 16-bit flat table for O(1) decode
// ---------------------------------------------------------------------------

void LjpegDecoder::build_huffman_lookup(const uint8_t* bits,
                                        const uint8_t* huffval, int nvals,
                                        HuffmanLookup& lookup) {
    std::memset(lookup.length, 0, sizeof(lookup.length));

    int code = 0;
    int p = 0;
    for (int k = 1; k <= 16; ++k) {
        for (int j = 0; j < bits[k - 1]; ++j) {
            if (p >= nvals) break;
            uint8_t sym = huffval[p++];
            int shift = 16 - k;
            uint32_t start = static_cast<uint32_t>(code) << shift;
            uint32_t end = start + (1u << shift);
            for (uint32_t idx = start; idx < end; ++idx) {
                lookup.symbol[idx] = sym;
                lookup.length[idx] = static_cast<uint8_t>(k);
            }
            code++;
        }
        code <<= 1;
    }
    lookup.valid = true;
}

// ---------------------------------------------------------------------------
// Main decode entry point
// ---------------------------------------------------------------------------

LjpegDecoder::Result LjpegDecoder::decode(const uint8_t* data, size_t size) {
    if (size < 4 || data[0] != 0xFF || data[1] != 0xD8) {
        throw std::runtime_error("Not a valid JPEG stream (no SOI marker)");
    }

    size_t pos = 2;  // skip SOI

    // Frame parameters
    int precision = 0;
    int frame_width = 0;
    int frame_height = 0;
    int num_components = 0;
    int component_ids[4] = {0, 0, 0, 0};

    // Huffman lookup tables (up to 4: DC tables 0-3)
    HuffmanLookup huff_tables[4];

    // Scan parameters
    int predictor = 1;
    int point_transform = 0;

    // Parse markers until we reach SOS
    while (pos + 1 < size) {
        if (data[pos] != 0xFF) {
            pos++;
            continue;
        }

        uint8_t marker = data[pos + 1];

        if (marker == 0xFF) { pos++; continue; }  // padding
        if (marker == 0xD9) break;                  // EOI
        if (marker >= 0xD0 && marker <= 0xD7) { pos += 2; continue; }  // RST

        // Read marker length
        if (pos + 3 >= size) break;
        uint16_t marker_len = (static_cast<uint16_t>(data[pos + 2]) << 8) |
                              data[pos + 3];
        size_t payload_start = pos + 4;
        size_t payload_end = pos + 2 + marker_len;

        if (marker == 0xC3) {
            // SOF3 — lossless Huffman frame
            if (payload_end > size) throw std::runtime_error("SOF3 truncated");
            precision = data[payload_start];
            frame_height = (static_cast<int>(data[payload_start + 1]) << 8) |
                           data[payload_start + 2];
            frame_width = (static_cast<int>(data[payload_start + 3]) << 8) |
                          data[payload_start + 4];
            num_components = data[payload_start + 5];

            if (num_components < 1 || num_components > 4) {
                throw std::runtime_error("Unsupported component count: " +
                    std::to_string(num_components));
            }
            if (precision < 2 || precision > 16) {
                throw std::runtime_error("Unsupported precision: " +
                    std::to_string(precision));
            }

            for (int c = 0; c < num_components; ++c) {
                size_t off = payload_start + 6 + c * 3;
                component_ids[c] = data[off];
            }
        } else if (marker == 0xC4) {
            // DHT — Define Huffman Table
            if (payload_end > size) throw std::runtime_error("DHT truncated");
            size_t off = payload_start;
            while (off + 17 <= payload_end) {
                uint8_t tc_th = data[off++];
                int table_class = tc_th >> 4;
                int table_id = tc_th & 0x0F;
                if (table_id > 3) throw std::runtime_error("Invalid Huffman table ID");

                uint8_t bits[16];
                std::memcpy(bits, data + off, 16);
                off += 16;

                int nvals = 0;
                for (int i = 0; i < 16; ++i) nvals += bits[i];

                if (off + nvals > payload_end)
                    throw std::runtime_error("DHT values truncated");

                uint8_t huffval[256];
                std::memcpy(huffval, data + off, nvals);
                off += nvals;

                if (table_class == 0) {
                    build_huffman_lookup(bits, huffval, nvals,
                                         huff_tables[table_id]);
                }
            }
        } else if (marker == 0xDA) {
            // SOS — Start of Scan
            if (payload_end > size) throw std::runtime_error("SOS truncated");
            int scan_components = data[payload_start];

            int comp_huff_table[4] = {0, 0, 0, 0};
            for (int c = 0; c < scan_components; ++c) {
                size_t off = payload_start + 1 + c * 2;
                int cid = data[off];
                int td = data[off + 1] >> 4;
                for (int i = 0; i < num_components; ++i) {
                    if (component_ids[i] == cid) {
                        comp_huff_table[i] = td;
                        break;
                    }
                }
            }

            size_t scan_tail = payload_start + 1 + scan_components * 2;
            if (scan_tail + 2 < payload_end) {
                predictor = data[scan_tail];
                int ah_al = data[scan_tail + 2];
                point_transform = ah_al & 0x0F;
            }

            // Entropy-coded data starts after SOS
            pos = payload_end;

            // --- Decode ---
            Result result;
            result.width = frame_width;
            result.height = frame_height;
            result.components = num_components;
            result.precision = precision;
            result.pixels.resize(static_cast<size_t>(frame_width) *
                                  frame_height * num_components);

            int mask = (1 << precision) - 1;
            int init_pred = 1 << (precision - 1 - point_transform);

            // Verify all needed Huffman tables are valid
            for (int c = 0; c < num_components; ++c) {
                if (!huff_tables[comp_huff_table[c]].valid) {
                    throw std::runtime_error(
                        "Huffman table not defined for component " +
                        std::to_string(c));
                }
            }

            BitReader br(data + pos, size - pos);

            // Per-component previous and current row buffers
            std::vector<std::vector<int>> prev_row(num_components,
                std::vector<int>(frame_width, 0));
            std::vector<std::vector<int>> curr_row(num_components,
                std::vector<int>(frame_width, 0));

            uint16_t* out = result.pixels.data();
            int fw = frame_width;
            int nc = num_components;

            for (int row = 0; row < frame_height; ++row) {
                for (int col = 0; col < frame_width; ++col) {
                    for (int c = 0; c < nc; ++c) {
                        int Px;

                        if (row == 0 && col == 0) {
                            Px = init_pred;
                        } else if (row == 0) {
                            Px = curr_row[c][col - 1];
                        } else if (col == 0) {
                            Px = prev_row[c][col];
                        } else {
                            int Ra = curr_row[c][col - 1];
                            int Rb = prev_row[c][col];
                            int Rc = prev_row[c][col - 1];

                            switch (predictor) {
                                case 1: Px = Ra; break;
                                case 2: Px = Rb; break;
                                case 3: Px = Rc; break;
                                case 4: Px = Ra + Rb - Rc; break;
                                case 5: Px = Ra + ((Rb - Rc) >> 1); break;
                                case 6: Px = Rb + ((Ra - Rc) >> 1); break;
                                case 7: Px = (Ra + Rb) >> 1; break;
                                default: Px = Ra; break;
                            }
                        }

                        // Fast Huffman decode via 16-bit lookup table
                        const HuffmanLookup& hl = huff_tables[comp_huff_table[c]];
                        int peek = br.peek_bits(16);
                        int ssss = hl.symbol[peek];
                        int clen = hl.length[peek];
                        if (clen == 0) {
                            throw std::runtime_error(
                                "Invalid Huffman code in LJPEG stream");
                        }
                        br.consume(clen);

                        // Receive-extend: decode difference value
                        int diff;
                        if (ssss == 0) {
                            diff = 0;
                        } else if (ssss == 16) {
                            diff = 32768;
                        } else {
                            int value = br.get_bits(ssss);
                            if (value < (1 << (ssss - 1))) {
                                value -= (1 << ssss) - 1;
                            }
                            diff = value;
                        }

                        // Reconstruct pixel value
                        int value = (Px + diff) & mask;
                        if (point_transform > 0) {
                            value <<= point_transform;
                        }

                        curr_row[c][col] = value;
                        out[(static_cast<size_t>(row) * fw + col) * nc + c] =
                            static_cast<uint16_t>(value);
                    }
                }
                std::swap(prev_row, curr_row);
            }

            return result;
        }

        pos = payload_end;
    }

    throw std::runtime_error("SOS marker not found in LJPEG stream");
}

} // namespace dng
