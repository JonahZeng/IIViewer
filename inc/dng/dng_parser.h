#pragma once

#include "dng/tiff_reader.h"
#include "dng/dng_image.h"

namespace dng {

// Parses a DNG file into a DngImage struct.
// Supports DNG files from multiple vendors:
//   - DJI Pocket 4: little-endian, uncompressed CFA Bayer, strip-based
//   - Apple ProRAW: big-endian, LJPEG-compressed LinearRaw, tile-based
//
// The parser does NOT apply any image-domain transforms (no linearization,
// no black-level subtraction, no CCM). It only reads + structures data.
class DngParser {
public:
    // Parse a DNG file. Throws std::runtime_error on failure.
    static DngImage parse(const std::string& path);

private:
    DngParser(TiffReader& reader);

    // Find the RAW IFD: walk SubIFDs from the main IFD, pick the one
    // with PhotometricInterpretation == CFA (32803) and NewSubfileType == 0.
    const IFD& find_raw_ifd(const std::vector<IFD>& sub_ifds);

    // Extract all metadata from the main IFD (global DNG tags)
    void parse_global_metadata(const IFD& main_ifd);

    // Extract pixel-level metadata from the RAW IFD
    void parse_raw_ifd(const IFD& raw_ifd);

    // Read the actual pixel data
    void read_raw_pixels(const IFD& raw_ifd);
    void read_raw_pixels_uncompressed(const IFD& raw_ifd);
    void read_raw_pixels_ljpeg(const IFD& raw_ifd);

    // Helpers for individual tag groups
    void parse_cfa(const IFD& ifd);
    void parse_levels(const IFD& ifd);
    void parse_color_matrices(const IFD& ifd);
    void parse_crop(const IFD& ifd);
    void parse_exposure(const IFD& main_ifd);
    void parse_opcodes(const IFD& raw_ifd);
    void parse_camera_info(const IFD& main_ifd);

    TiffReader& reader_;
    DngImage result_;
};

} // namespace dng
