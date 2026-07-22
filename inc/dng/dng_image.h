#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace dng {

// CFA color codes (DNG spec: CFAPattern tag values)
// 0=Red, 1=Green, 2=Blue
enum class CfaColor : uint8_t {
    RED   = 0,
    GREEN = 1,
    BLUE  = 2,
};

// Calibration illuminant (LightSource values, DNG spec)
enum class Illuminant : uint16_t {
    DAYLIGHT      = 1,
    FLUORESCENT   = 2,
    TUNGSTEN      = 3,
    FLASH         = 4,
    CLOUDY        = 10,
    SHADE         = 11,
    DAYLIGHT_FL   = 12,
    DAY_WHITE_FL  = 13,
    STANDARD_A    = 17,   // 2856K
    STANDARD_B    = 18,
    STANDARD_C    = 19,
    D55           = 20,
    D65           = 21,   // 6500K
    D75           = 22,
    D50           = 23,
    OTHER         = 255,
};

// 3x3 matrix stored as row-major float[9]
struct Matrix3x3 {
    float m[9];

    float& operator()(int r, int c) { return m[r * 3 + c]; }
    float  operator()(int r, int c) const { return m[r * 3 + c]; }
};

// NoiseProfile: variance = slope * value + offset (DNG 1.2+)
struct NoiseProfile {
    double slope;
    double offset;
    bool present = false;
};

// The parsed result of a DNG file — everything the PyTorch Dataset needs.
// Raw pixel data is kept separate (allocated as a contiguous buffer)
// so it can be wrapped into a torch tensor without copying.
struct DngImage {
    // --- Raw pixel data ---
    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t bits_per_sample = 0;
    uint16_t samples_per_pixel = 0;
    uint32_t compression = 0;          // TIFF Compression tag value
    uint16_t photometric_interpretation = 0;  // 32803=CFA, 34892=LinearRaw
    std::vector<uint8_t> raw_pixels;   // packed pixel data, row-major
    // For CFA (1 channel): width*height*2 bytes (uint16)
    // For LinearRaw (N channels): width*height*N*2 bytes (uint16, interleaved)

    // --- Tiling ---
    bool is_tiled = false;
    uint32_t tile_width = 0;
    uint32_t tile_length = 0;

    // --- CFA geometry ---
    uint16_t cfa_repeat_rows = 0;      // e.g. 2
    uint16_t cfa_repeat_cols = 0;      // e.g. 2
    std::vector<uint8_t> cfa_pattern;  // size = rows*cols, CfaColor values
    uint16_t cfa_layout = 0;           // 1=rectangular, 2=staggered A, ...

    // --- Black / White levels ---
    uint16_t black_level_repeat_rows = 0;
    uint16_t black_level_repeat_cols = 0;
    std::vector<double> black_level;   // per-CFA-position, size = repeat_rows*repeat_cols
    uint32_t white_level = 0;
    std::vector<uint32_t> white_level_per_sample;  // per-sample, if multiple
    std::vector<uint16_t> linearization_table; // empty if not present

    // --- Color metadata ---
    Matrix3x3 color_matrix_1;          // camera -> XYZ at illuminant 1
    Matrix3x3 color_matrix_2;          // camera -> XYZ at illuminant 2
    bool has_color_matrix_2 = false;
    Illuminant calibration_illuminant_1 = Illuminant::OTHER;
    Illuminant calibration_illuminant_2 = Illuminant::OTHER;

    Matrix3x3 forward_matrix_1;
    Matrix3x3 forward_matrix_2;
    bool has_forward_matrix_1 = false;
    bool has_forward_matrix_2 = false;

    // White balance as shot: diagonal scaling factors (r, g, b)
    std::vector<double> as_shot_neutral; // size 3

    // --- Active area & crop ---
    // ActiveArea: [top, left, bottom, right]
    uint32_t active_area[4] = {0, 0, 0, 0};
    bool has_active_area = false;
    // DefaultCropOrigin and DefaultCropSize as rationals
    double default_crop_origin[2] = {0.0, 0.0};
    double default_crop_size[2] = {0.0, 0.0};

    // --- Exposure metadata ---
    double baseline_exposure = 0.0;
    double iso_speed = 0.0;
    double exposure_time = 0.0;   // seconds
    double f_number = 0.0;
    double focal_length = 0.0;    // mm

    // --- Noise ---
    NoiseProfile noise_profile;

    // --- Opcode lists (raw bytes, not yet parsed) ---
    std::vector<uint8_t> opcode_list_1;
    std::vector<uint8_t> opcode_list_2;
    std::vector<uint8_t> opcode_list_3;

    // --- Camera info ---
    std::string make;
    std::string model;
    std::string unique_camera_model;
    std::string serial_number;

    // --- DNG version ---
    uint8_t dng_version[4] = {0, 0, 0, 0};
};

} // namespace dng
