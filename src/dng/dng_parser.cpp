#include "dng/dng_parser.h"
#include "dng/ljpeg_decoder.h"

#include <cstring>

namespace dng {

// TIFF tag IDs
namespace tag {
    constexpr uint16_t NewSubfileType        = 254;
    constexpr uint16_t ImageWidth            = 256;
    constexpr uint16_t ImageLength           = 257;
    constexpr uint16_t BitsPerSample         = 258;
    constexpr uint16_t Compression           = 259;
    constexpr uint16_t PhotometricInterpretation = 262;
    constexpr uint16_t Make                  = 271;
    constexpr uint16_t Model                 = 272;
    constexpr uint16_t StripOffsets          = 273;
    constexpr uint16_t Orientation           = 274;
    constexpr uint16_t SamplesPerPixel       = 277;
    constexpr uint16_t RowsPerStrip          = 278;
    constexpr uint16_t StripByteCounts       = 279;
    constexpr uint16_t PlanarConfiguration   = 284;
    constexpr uint16_t TileWidth             = 322;
    constexpr uint16_t TileLength            = 323;
    constexpr uint16_t TileOffsets           = 324;
    constexpr uint16_t TileByteCounts        = 325;
    constexpr uint16_t SubIFDs               = 330;
    constexpr uint16_t CFARepeatPatternDim   = 33421;
    constexpr uint16_t CFAPattern            = 33422;

    // EXIF tags (referenced via ExifIFD pointer)
    constexpr uint16_t ExposureTime          = 33434;  // via ExifIFD
    constexpr uint16_t FNumber               = 33437;  // via ExifIFD
    constexpr uint16_t ExifTag               = 34665;  // pointer to ExifIFD
    constexpr uint16_t FocalLength           = 37386;  // via ExifIFD
    constexpr uint16_t ISOSpeedRatings       = 34855;  // via ExifIFD

    // DNG tags
    constexpr uint16_t DNGVersion            = 50706;
    constexpr uint16_t DNGBackwardVersion    = 50707;
    constexpr uint16_t UniqueCameraModel     = 50708;
    constexpr uint16_t CFAPlaneColor         = 50710;
    constexpr uint16_t CFALayout             = 50711;
    constexpr uint16_t LinearizationTable    = 50712;
    constexpr uint16_t BlackLevelRepeatDim   = 50713;
    constexpr uint16_t BlackLevel            = 50714;
    constexpr uint16_t WhiteLevel            = 50717;
    constexpr uint16_t DefaultScale          = 50718;
    constexpr uint16_t DefaultCropOrigin     = 50719;
    constexpr uint16_t DefaultCropSize       = 50720;
    constexpr uint16_t ColorMatrix1          = 50721;
    constexpr uint16_t ColorMatrix2          = 50722;
    constexpr uint16_t CameraCalibration1    = 50723;
    constexpr uint16_t CameraCalibration2    = 50724;
    constexpr uint16_t AnalogBalance          = 50727;
    constexpr uint16_t AsShotNeutral         = 50728;
    constexpr uint16_t ForwardMatrix1        = 50934;
    constexpr uint16_t ForwardMatrix2        = 50935;
    constexpr uint16_t AsShotWhiteXY         = 50729;
    constexpr uint16_t BaselineExposure      = 50730;
    constexpr uint16_t BaselineNoise         = 50731;
    constexpr uint16_t BaselineSharpness     = 50732;
    constexpr uint16_t LinearResponseLimit   = 50734;
    constexpr uint16_t CameraSerialNumber    = 50735;
    constexpr uint16_t CalibrationIlluminant1 = 50778;
    constexpr uint16_t CalibrationIlluminant2 = 50779;
    constexpr uint16_t BestQualityScale      = 50780;
    constexpr uint16_t ActiveArea            = 50829;
    constexpr uint16_t OpcodeList1           = 51008;
    constexpr uint16_t OpcodeList2           = 51009;
    constexpr uint16_t OpcodeList3           = 51022;
    constexpr uint16_t NoiseProfile          = 51041;

    // PhotometricInterpretation values
    constexpr uint16_t PHOTOMETRIC_CFA       = 32803;
    constexpr uint16_t PHOTOMETRIC_LINEAR_RAW = 34892;

    // Compression values
    constexpr uint16_t COMPRESSION_NONE      = 1;
    constexpr uint16_t COMPRESSION_LJPEG     = 7;
    constexpr uint16_t COMPRESSION_LOSSY     = 34892;
} // namespace tag

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

DngImage DngParser::parse(const std::string& path) {
    TiffReader reader(path);

    // Read first IFD (offset at bytes 4-8)
    uint32_t first_ifd_offset = reader.read_u32(4);
    IFD main_ifd = reader.parse_ifd(first_ifd_offset);

    // Parse SubIFDs
    std::vector<IFD> sub_ifds;
    const TiffTag* subifd_tag = reader.find_tag(main_ifd, tag::SubIFDs);
    if (subifd_tag) {
        auto offsets = reader.get_u32_array(*subifd_tag);
        for (uint32_t off : offsets) {
            sub_ifds.push_back(reader.parse_ifd(off));
        }
    }

    DngParser parser(reader);
    parser.parse_global_metadata(main_ifd);

    // Find the RAW IFD among SubIFDs
    const IFD& raw_ifd = parser.find_raw_ifd(sub_ifds);
    parser.parse_raw_ifd(raw_ifd);

    return std::move(parser.result_);
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

DngParser::DngParser(TiffReader& reader)
    : reader_(reader) {
}

// ---------------------------------------------------------------------------
// Find RAW IFD
// ---------------------------------------------------------------------------

const IFD& DngParser::find_raw_ifd(const std::vector<IFD>& sub_ifds) {
    // Look for SubIFD with PhotometricInterpretation == CFA (32803) or
    // LinearRaw (34892) and NewSubfileType == 0 (full resolution).
    for (const auto& ifd : sub_ifds) {
        const TiffTag* photometric = reader_.find_tag(ifd, tag::PhotometricInterpretation);
        const TiffTag* subfile_type = reader_.find_tag(ifd, tag::NewSubfileType);

        if (photometric) {
            uint16_t pi = reader_.get_u16(*photometric);
            if (pi == tag::PHOTOMETRIC_CFA || pi == tag::PHOTOMETRIC_LINEAR_RAW) {
                if (!subfile_type || reader_.get_u32(*subfile_type) == 0) {
                    return ifd;
                }
            }
        }
    }
    // Fallback: first SubIFD with CFA or LinearRaw photometric
    for (const auto& ifd : sub_ifds) {
        const TiffTag* photometric = reader_.find_tag(ifd, tag::PhotometricInterpretation);
        if (photometric) {
            uint16_t pi = reader_.get_u16(*photometric);
            if (pi == tag::PHOTOMETRIC_CFA || pi == tag::PHOTOMETRIC_LINEAR_RAW) {
                return ifd;
            }
        }
    }
    throw std::runtime_error("No CFA or LinearRaw IFD found in DNG SubIFDs");
}

// ---------------------------------------------------------------------------
// Parse global metadata (from main IFD)
// ---------------------------------------------------------------------------

void DngParser::parse_global_metadata(const IFD& main_ifd) {
    parse_color_matrices(main_ifd);
    parse_exposure(main_ifd);
    parse_camera_info(main_ifd);

    // AsShotNeutral
    const TiffTag* asn = reader_.find_tag(main_ifd, tag::AsShotNeutral);
    if (asn) {
        result_.as_shot_neutral = reader_.get_rational_array(*asn);
    }

    // BaselineExposure
    const TiffTag* be = reader_.find_tag(main_ifd, tag::BaselineExposure);
    if (be) {
        result_.baseline_exposure = (be->type == TiffType::SRATIONAL) ? reader_.get_srational(*be) : reader_.get_rational(*be);
    }

    // CalibrationIlluminants
    const TiffTag* ci1 = reader_.find_tag(main_ifd, tag::CalibrationIlluminant1);
    if (ci1) {
        result_.calibration_illuminant_1 = static_cast<Illuminant>(reader_.get_u16(*ci1));
    }
    const TiffTag* ci2 = reader_.find_tag(main_ifd, tag::CalibrationIlluminant2);
    if (ci2) {
        result_.calibration_illuminant_2 = static_cast<Illuminant>(reader_.get_u16(*ci2));
    }

    // NoiseProfile
    const TiffTag* np = reader_.find_tag(main_ifd, tag::NoiseProfile);
    if (np && np->count >= 2) {
        result_.noise_profile.present = true;
        auto vals = reader_.get_f64_array(*np);  // NoiseProfile is DOUBLE
        if (vals.size() >= 2) {
            result_.noise_profile.slope = vals[0];
            result_.noise_profile.offset = vals[1];
        }
    }

    // DNGVersion
    const TiffTag* ver = reader_.find_tag(main_ifd, tag::DNGVersion);
    if (ver) {
        auto bytes = reader_.get_bytes(*ver);
        for (size_t i = 0; i < 4 && i < bytes.size(); ++i) {
            result_.dng_version[i] = bytes[i];
        }
    }
}

// ---------------------------------------------------------------------------
// Parse RAW IFD (pixel-level metadata + data)
// ---------------------------------------------------------------------------

void DngParser::parse_raw_ifd(const IFD& raw_ifd) {
    // Image dimensions
    const TiffTag* w = reader_.find_tag(raw_ifd, tag::ImageWidth);
    const TiffTag* h = reader_.find_tag(raw_ifd, tag::ImageLength);
    if (w) result_.width = reader_.get_u32(*w);
    if (h) result_.height = reader_.get_u32(*h);

    // Bits per sample
    const TiffTag* bps = reader_.find_tag(raw_ifd, tag::BitsPerSample);
    if (bps) result_.bits_per_sample = reader_.get_u16(*bps);

    // Samples per pixel
    const TiffTag* spp = reader_.find_tag(raw_ifd, tag::SamplesPerPixel);
    if (spp) result_.samples_per_pixel = reader_.get_u16(*spp);

    // Compression
    const TiffTag* comp = reader_.find_tag(raw_ifd, tag::Compression);
    if (comp) result_.compression = reader_.get_u16(*comp);

    // PhotometricInterpretation
    const TiffTag* photometric = reader_.find_tag(raw_ifd, tag::PhotometricInterpretation);
    if (photometric) {
        result_.photometric_interpretation = reader_.get_u16(*photometric);
    }

    // Tiling info
    const TiffTag* tw = reader_.find_tag(raw_ifd, tag::TileWidth);
    const TiffTag* tl = reader_.find_tag(raw_ifd, tag::TileLength);
    if (tw && tl) {
        result_.is_tiled = true;
        result_.tile_width = reader_.get_u32(*tw);
        result_.tile_length = reader_.get_u32(*tl);
    }

    // CFA info (only present for CFA photometric, not LinearRaw)
    if (result_.photometric_interpretation == tag::PHOTOMETRIC_CFA) {
        parse_cfa(raw_ifd);
    }

    // Black/White levels
    parse_levels(raw_ifd);

    // Crop and active area
    parse_crop(raw_ifd);

    // Opcodes
    parse_opcodes(raw_ifd);

    // NoiseProfile may be on the raw IFD (Apple) or main IFD (DJI).
    // Only read from raw IFD if not already set from main IFD.
    if (!result_.noise_profile.present) {
        const TiffTag* np = reader_.find_tag(raw_ifd, tag::NoiseProfile);
        if (np && np->count >= 2) {
            result_.noise_profile.present = true;
            auto vals = reader_.get_f64_array(*np);
            if (vals.size() >= 2) {
                result_.noise_profile.slope = vals[0];
                result_.noise_profile.offset = vals[1];
            }
        }
    }

    // Read actual pixel data
    read_raw_pixels(raw_ifd);
}

// ---------------------------------------------------------------------------
// CFA pattern parsing
// ---------------------------------------------------------------------------

void DngParser::parse_cfa(const IFD& ifd) {
    const TiffTag* dim = reader_.find_tag(ifd, tag::CFARepeatPatternDim);
    if (dim) {
        auto dims = reader_.get_u16_array(*dim);
        if (dims.size() >= 2) {
            result_.cfa_repeat_rows = dims[0];
            result_.cfa_repeat_cols = dims[1];
        }
    }

    const TiffTag* pat = reader_.find_tag(ifd, tag::CFAPattern);
    if (pat) {
        result_.cfa_pattern = reader_.get_bytes(*pat);
    }

    const TiffTag* layout = reader_.find_tag(ifd, tag::CFALayout);
    if (layout) {
        result_.cfa_layout = reader_.get_u16(*layout);
    }
}

// ---------------------------------------------------------------------------
// Black/White level parsing
// ---------------------------------------------------------------------------

void DngParser::parse_levels(const IFD& ifd) {
    const TiffTag* blr = reader_.find_tag(ifd, tag::BlackLevelRepeatDim);
    if (blr) {
        auto dims = reader_.get_u16_array(*blr);
        if (dims.size() >= 2) {
            result_.black_level_repeat_rows = dims[0];
            result_.black_level_repeat_cols = dims[1];
        }
    }

    const TiffTag* bl = reader_.find_tag(ifd, tag::BlackLevel);
    if (bl) {
        // BlackLevel can be SHORT, LONG, or RATIONAL
        if (bl->type == TiffType::RATIONAL) {
            result_.black_level = reader_.get_rational_array(*bl);
        } else if (bl->type == TiffType::SHORT) {
            auto vals = reader_.get_u16_array(*bl);
            result_.black_level.assign(vals.begin(), vals.end());
        } else if (bl->type == TiffType::LONG) {
            auto vals = reader_.get_u32_array(*bl);
            result_.black_level.assign(vals.begin(), vals.end());
        }
    }

    const TiffTag* wl = reader_.find_tag(ifd, tag::WhiteLevel);
    if (wl) {
        if (wl->type == TiffType::SHORT) {
            auto vals = reader_.get_u16_array(*wl);
            for (uint16_t v : vals) {
                result_.white_level_per_sample.push_back(v);
            }
            if (!vals.empty()) {
                result_.white_level = vals[0];
            }
        } else {
            auto vals = reader_.get_u32_array(*wl);
            for (uint32_t v : vals) {
                result_.white_level_per_sample.push_back(v);
            }
            if (!vals.empty()) {
                result_.white_level = vals[0];
            }
        }
    }

    const TiffTag* lt = reader_.find_tag(ifd, tag::LinearizationTable);
    if (lt) {
        result_.linearization_table = reader_.get_u16_array(*lt);
    }
}

// ---------------------------------------------------------------------------
// Color matrix parsing
// ---------------------------------------------------------------------------

void DngParser::parse_color_matrices(const IFD& ifd) {
    // ColorMatrix1: 3x3 RATIONAL (9 values) — camera native -> XYZ
    const TiffTag* cm1 = reader_.find_tag(ifd, tag::ColorMatrix1);
    if (cm1) {
        auto vals = (cm1->type == TiffType::SRATIONAL) ? reader_.get_srational_array(*cm1) : reader_.get_rational_array(*cm1);
        if (vals.size() >= 9) {
            for (int i = 0; i < 9; ++i) {
                result_.color_matrix_1.m[i] = static_cast<float>(vals[i]);
            }
        }
    }

    // ColorMatrix2
    const TiffTag* cm2 = reader_.find_tag(ifd, tag::ColorMatrix2);
    if (cm2) {
        auto vals = (cm2->type == TiffType::SRATIONAL) ? reader_.get_srational_array(*cm2) : reader_.get_rational_array(*cm2);
        if (vals.size() >= 9) {
            for (int i = 0; i < 9; ++i) {
                result_.color_matrix_2.m[i] = static_cast<float>(vals[i]);
            }
            result_.has_color_matrix_2 = true;
        }
    }

    // ForwardMatrix1 (XYZ -> camera, or camera -> XYZ depending on spec)
    const TiffTag* fm1 = reader_.find_tag(ifd, tag::ForwardMatrix1);
    if (fm1) {
        auto vals = (fm1->type == TiffType::SRATIONAL) ? reader_.get_srational_array(*fm1) : reader_.get_rational_array(*fm1);
        if (vals.size() >= 9) {
            for (int i = 0; i < 9; ++i) {
                result_.forward_matrix_1.m[i] = static_cast<float>(vals[i]);
            }
            result_.has_forward_matrix_1 = true;
        }
    }

    // ForwardMatrix2
    const TiffTag* fm2 = reader_.find_tag(ifd, tag::ForwardMatrix2);
    if (fm2) {
        auto vals = (fm2->type == TiffType::SRATIONAL) ? reader_.get_srational_array(*fm2) : reader_.get_rational_array(*fm2);
        if (vals.size() >= 9) {
            for (int i = 0; i < 9; ++i) {
                result_.forward_matrix_2.m[i] = static_cast<float>(vals[i]);
            }
            result_.has_forward_matrix_2 = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Crop and active area
// ---------------------------------------------------------------------------

void DngParser::parse_crop(const IFD& ifd) {
    const TiffTag* aa = reader_.find_tag(ifd, tag::ActiveArea);
    if (aa) {
        auto vals = reader_.get_u32_array(*aa);  // LONG[4]: top, left, bottom, right
        if (vals.size() >= 4) {
            result_.active_area[0] = vals[0];
            result_.active_area[1] = vals[1];
            result_.active_area[2] = vals[2];
            result_.active_area[3] = vals[3];
            result_.has_active_area = true;
        }
    }

    const TiffTag* dco = reader_.find_tag(ifd, tag::DefaultCropOrigin);
    if (dco) {
        auto vals = reader_.get_rational_array(*dco);
        if (vals.size() >= 2) {
            result_.default_crop_origin[0] = vals[0];
            result_.default_crop_origin[1] = vals[1];
        }
    }

    const TiffTag* dcs = reader_.find_tag(ifd, tag::DefaultCropSize);
    if (dcs) {
        auto vals = reader_.get_rational_array(*dcs);
        if (vals.size() >= 2) {
            result_.default_crop_size[0] = vals[0];
            result_.default_crop_size[1] = vals[1];
        }
    }
}

// ---------------------------------------------------------------------------
// Exposure metadata (from ExifIFD, pointed to by ExifTag)
// ---------------------------------------------------------------------------

void DngParser::parse_exposure(const IFD& main_ifd) {
    // ExifIFD is a sub-IFD pointed to by tag 34665
    const TiffTag* exif_ptr = reader_.find_tag(main_ifd, tag::ExifTag);
    if (!exif_ptr) return;

    uint32_t exif_offset = reader_.get_u32(*exif_ptr);
    IFD exif_ifd = reader_.parse_ifd(exif_offset);

    // ExposureTime (RATIONAL)
    const TiffTag* et = reader_.find_tag(exif_ifd, tag::ExposureTime);
    if (et) {
        result_.exposure_time = reader_.get_rational(*et);
    }

    // FNumber (RATIONAL)
    const TiffTag* fn = reader_.find_tag(exif_ifd, tag::FNumber);
    if (fn) {
        result_.f_number = reader_.get_rational(*fn);
    }

    // ISOSpeedRatings (SHORT)
    const TiffTag* iso = reader_.find_tag(exif_ifd, tag::ISOSpeedRatings);
    if (iso) {
        result_.iso_speed = reader_.get_u16(*iso);
    }

    // FocalLength (RATIONAL)
    const TiffTag* fl = reader_.find_tag(exif_ifd, tag::FocalLength);
    if (fl) {
        result_.focal_length = reader_.get_rational(*fl);
    }
}

// ---------------------------------------------------------------------------
// Opcode lists
// ---------------------------------------------------------------------------

void DngParser::parse_opcodes(const IFD& raw_ifd) {
    const TiffTag* op1 = reader_.find_tag(raw_ifd, tag::OpcodeList1);
    if (op1) result_.opcode_list_1 = reader_.get_undefined(*op1);

    const TiffTag* op2 = reader_.find_tag(raw_ifd, tag::OpcodeList2);
    if (op2) result_.opcode_list_2 = reader_.get_undefined(*op2);

    const TiffTag* op3 = reader_.find_tag(raw_ifd, tag::OpcodeList3);
    if (op3) result_.opcode_list_3 = reader_.get_undefined(*op3);
}

// ---------------------------------------------------------------------------
// Camera info
// ---------------------------------------------------------------------------

void DngParser::parse_camera_info(const IFD& main_ifd) {
    const TiffTag* mk = reader_.find_tag(main_ifd, tag::Make);
    if (mk) result_.make = reader_.get_ascii(*mk);

    const TiffTag* mdl = reader_.find_tag(main_ifd, tag::Model);
    if (mdl) result_.model = reader_.get_ascii(*mdl);

    const TiffTag* ucm = reader_.find_tag(main_ifd, tag::UniqueCameraModel);
    if (ucm) result_.unique_camera_model = reader_.get_ascii(*ucm);

    const TiffTag* sn = reader_.find_tag(main_ifd, tag::CameraSerialNumber);
    if (sn) result_.serial_number = reader_.get_ascii(*sn);
}

// ---------------------------------------------------------------------------
// Read raw pixel data — supports uncompressed strips and LJPEG tiles
// ---------------------------------------------------------------------------

void DngParser::read_raw_pixels(const IFD& raw_ifd) {
    if (result_.compression == tag::COMPRESSION_NONE) {
        read_raw_pixels_uncompressed(raw_ifd);
    } else if (result_.compression == tag::COMPRESSION_LJPEG) {
        read_raw_pixels_ljpeg(raw_ifd);
    } else {
        throw std::runtime_error(
            "Unsupported compression: " + std::to_string(result_.compression)
            + ". Only uncompressed (1) and LJPEG (7) are supported.");
    }
}

// ---------------------------------------------------------------------------
// Read uncompressed raw pixel data (strips)
// ---------------------------------------------------------------------------

void DngParser::read_raw_pixels_uncompressed(const IFD& raw_ifd) {
    if (result_.bits_per_sample != 16) {
        throw std::runtime_error(
            "Unsupported BitsPerSample for uncompressed: "
            + std::to_string(result_.bits_per_sample)
            + ". Only 16-bit is supported.");
    }

    const TiffTag* offsets_tag = reader_.find_tag(raw_ifd, tag::StripOffsets);
    const TiffTag* counts_tag = reader_.find_tag(raw_ifd, tag::StripByteCounts);
    if (!offsets_tag || !counts_tag) {
        throw std::runtime_error("Missing StripOffsets or StripByteCounts");
    }

    std::vector<uint32_t> offsets = reader_.get_u32_array(*offsets_tag);
    std::vector<uint32_t> counts = reader_.get_u32_array(*counts_tag);

    size_t bytes_per_pixel = result_.samples_per_pixel * (result_.bits_per_sample / 8);
    size_t total_bytes = static_cast<size_t>(result_.width)
                       * static_cast<size_t>(result_.height)
                       * bytes_per_pixel;

    result_.raw_pixels.resize(total_bytes);
    size_t dst_offset = 0;
    for (size_t i = 0; i < offsets.size() && i < counts.size(); ++i) {
        size_t strip_bytes = counts[i];
        if (dst_offset + strip_bytes > total_bytes) {
            strip_bytes = total_bytes - dst_offset;
        }
        reader_.read_at(offsets[i], result_.raw_pixels.data() + dst_offset, strip_bytes);
        dst_offset += strip_bytes;
    }

    // Byte-swap if file is big-endian (host is assumed little-endian)
    if (!reader_.is_little_endian()) {
        uint16_t* pixels = reinterpret_cast<uint16_t*>(result_.raw_pixels.data());
        size_t count = total_bytes / 2;
        for (size_t i = 0; i < count; ++i) {
            uint16_t& v = pixels[i];
            v = static_cast<uint16_t>((v >> 8) | (v << 8));
        }
    }
}

// ---------------------------------------------------------------------------
// Read LJPEG-compressed raw pixel data (tiles)
// ---------------------------------------------------------------------------

void DngParser::read_raw_pixels_ljpeg(const IFD& raw_ifd) {
    const TiffTag* offsets_tag = reader_.find_tag(raw_ifd, tag::TileOffsets);
    const TiffTag* counts_tag = reader_.find_tag(raw_ifd, tag::TileByteCounts);

    if (!offsets_tag || !counts_tag) {
        // Fall back to strips for LJPEG (some DNGs use strips)
        offsets_tag = reader_.find_tag(raw_ifd, tag::StripOffsets);
        counts_tag = reader_.find_tag(raw_ifd, tag::StripByteCounts);
    }

    if (!offsets_tag || !counts_tag) {
        throw std::runtime_error(
            "Missing TileOffsets/TileByteCounts or StripOffsets/StripByteCounts");
    }

    std::vector<uint32_t> offsets = reader_.get_u32_array(*offsets_tag);
    std::vector<uint32_t> counts = reader_.get_u32_array(*counts_tag);

    uint32_t spp = result_.samples_per_pixel > 0 ? result_.samples_per_pixel : 1;
    size_t total_pixels = static_cast<size_t>(result_.width)
                        * static_cast<size_t>(result_.height)
                        * spp;
    // LJPEG-decoded values are stored as uint16 regardless of precision
    result_.raw_pixels.resize(total_pixels * 2);

    uint16_t* dst = reinterpret_cast<uint16_t*>(result_.raw_pixels.data());

    // Compute tile grid dimensions
    uint32_t tw = result_.tile_width;
    uint32_t th = result_.tile_length;
    if (tw == 0) tw = result_.width;
    if (th == 0) th = result_.height;

    uint32_t tiles_across = (result_.width + tw - 1) / tw;

    // Buffer for reading compressed tile data
    for (size_t i = 0; i < offsets.size() && i < counts.size(); ++i) {
        std::vector<uint8_t> compressed(counts[i]);
        reader_.read_at(offsets[i], compressed.data(), counts[i]);

        LjpegDecoder::Result tile = LjpegDecoder::decode(
            compressed.data(), compressed.size());

        // Determine tile position in the image
        uint32_t ty = static_cast<uint32_t>(i) / tiles_across;
        uint32_t tx = static_cast<uint32_t>(i) % tiles_across;

        uint32_t dst_x = tx * tw;
        uint32_t dst_y = ty * th;

        // Copy decoded tile pixels to the output image
        int tile_w = tile.width;
        int tile_h = tile.height;
        int tile_c = tile.components;

        for (int row = 0; row < tile_h; ++row) {
            uint32_t out_row = dst_y + static_cast<uint32_t>(row);
            if (out_row >= result_.height) break;

            for (int col = 0; col < tile_w; ++col) {
                uint32_t out_col = dst_x + static_cast<uint32_t>(col);
                if (out_col >= result_.width) break;

                size_t src_idx = (static_cast<size_t>(row) * tile_w + col) * tile_c;
                size_t dst_idx = (static_cast<size_t>(out_row) * result_.width + out_col) * spp;

                for (uint32_t c = 0; c < spp && c < static_cast<uint32_t>(tile_c); ++c) {
                    dst[dst_idx + c] = tile.pixels[src_idx + c];
                }
            }
        }
    }
}

} // namespace dng
