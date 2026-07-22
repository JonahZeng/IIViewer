#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>

namespace dng {

// Minimal lossless JPEG (SOF3) decoder for DNG LJPEG-compressed tiles/strips.
//
// Supports:
//   - SOF3 (lossless Huffman) frames
//   - Up to 4 components (interleaved)
//   - 2-16 bit precision
//   - Predictors 1-7 (1=left, 2=above, 3=upper-left, 4-7=various)
//   - Point transform (successive approximation low bits)
//   - 0xFF00 byte stuffing
//   - Restart markers (RST0-RST7) — resets prediction state
class LjpegDecoder {
public:
    struct Result {
        std::vector<uint16_t> pixels;  // interleaved: [c0,c1,..,cN, c0,c1,..,cN, ...]
        int width = 0;
        int height = 0;
        int components = 0;
        int precision = 0;  // bits per sample
    };

    static Result decode(const uint8_t* data, size_t size);

private:
    // 16-bit flat Huffman lookup table for O(1) decoding.
    // For each possible 16-bit peek value, stores the decoded symbol
    // and the code length (number of bits to consume).
    struct HuffmanLookup {
        uint8_t symbol[65536];
        uint8_t length[65536];  // 0 = invalid
        bool valid = false;
    };

    // MSB-first bit reader with 64-bit buffer and JPEG 0xFF00 unstuffing.
    class BitReader {
    public:
        BitReader(const uint8_t* data, size_t size);
        int get_bit();
        int get_bits(int n);
        int peek_bits(int n);
        void consume(int n);
        bool marker_found() const { return marker_found_; }
        void clear_marker() { marker_found_ = false; }
        int bits() const { return bits_in_buf_; }
    private:
        const uint8_t* data_;
        size_t size_;
        size_t pos_;
        uint64_t buf_;
        int bits_in_buf_;
        bool marker_found_;
        void fill();
    };

    static void build_huffman_lookup(const uint8_t* bits,
                                     const uint8_t* huffval, int nvals,
                                     HuffmanLookup& lookup);
};

} // namespace dng
