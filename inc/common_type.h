#ifndef _COMMON_TYPE_H_
#define _COMMON_TYPE_H_
enum OpenedImageType
{
    NORMAL_IMG = 1,
    RAW_IMG = 2,
    PNM_IMG = 3,
    PGM_IMG = 4,
    YUV_IMG = 5,
    UNKNOW_IMG = -1
};

enum MouseActionType
{
    NONE_ACTION = -1,
    PAINT_ROI_ACTION = 0,
    DRAG_IMG_ACTION = 1
};

enum YuvType
{
    YUV444_INTERLEAVE = 0,
    YUV444_PLANAR = 1,
    YUV422_UYVY = 2,
    YUV422_YUYV = 3,
    YUV420_NV12 = 4,
    YUV420_NV21 = 5,
    YUV420P_YU12 = 6,
    YUV420P_YV12 = 7,
    YUV400 = 8,
    YUV422_YUYV_AS1 = 9,
    YUV422_UYVY_AS1 = 10,
    YUV_UNKNOW = -1
};

enum YuvRatioType
{
    UV444 = 0,
    UV422 = 1,
    UV420 = 2,
    UV400 = 3,
};

enum BayerPatternType
{
    RGGB = 0,
    GRBG = 1,
    GBRG = 2,
    BGGR = 3,
    RGGIR = 4,
    BGGIR = 5,
    GRIRG = 6,
    GBIRG = 7,
    GIRRG = 8,
    GIRBG = 9,
    IRGGR = 10,
    IRGGB = 11,
    MONO = 12,
    BAYER_UNKNOW = -1
};
enum BayerPixelType
{
    PIX_R = 0,
    PIX_GR = 1,
    PIX_GB = 2,
    PIX_B = 3,
    PIX_IR = 4,
    PIX_Y = 5,
    PIX_UNKNOW = -1
};
enum ByteOrderType
{
    RAW_LITTLE_ENDIAN = 0,
    RAW_BIG_ENDIAN = 1,
};

#define CLIP3(a, mi, ma) (a < mi ? mi : (a > ma ? ma : a))
#endif