#include "ColorSpaceCvt.h"
#include <cstdint>

constexpr int BIT8 = 8;
constexpr int BIT16 = 16;

void convertYUV2RGB888(const unsigned char *yuvBuf, unsigned char *rgb888Buf, int bitDepth, int width, int height, YuvType yuv_tp, int wholepixperline) // NOLINT(readability-function-cognitive-complexity)
{
    if (yuv_tp == YuvType::YUV444_INTERLEAVE)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width * 3) + (j * 3) + 0]) >> (bitDepth - BIT8));
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width * 3) + (j * 3) + 1]) >> (bitDepth - BIT8));
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width * 3) + (j * 3) + 2]) >> (bitDepth - BIT8));
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width * 3) + (j * 3) + 0];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(i * width * 3) + (j * 3) + 1];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(i * width * 3) + (j * 3) + 2];
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV444_PLANAR)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width) + j] >> (bitDepth - BIT8));
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + (i * width) + j] >> (bitDepth - BIT8));
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height * 2) + (i * width) + j] >> (bitDepth - BIT8));
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width) + j];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(width * height) + (i * width) + j];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(width * height * 2) + (i * width) + j];
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV422_UYVY)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2) + 1] >> (bitDepth - BIT8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2)] >> (bitDepth - BIT8));     // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2) + 2] >> (bitDepth - BIT8)); // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2) - 2] >> (bitDepth - BIT8)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2)] >> (bitDepth - BIT8));     // v
                    }
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width * 2) + (j * 2) + 1];
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(i * width * 2) + (j * 2)];     // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(i * width * 2) + (j * 2) + 2]; // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(i * width * 2) + (j * 2) - 2]; // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(i * width * 2) + (j * 2)];     // v
                    }
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV422_YUYV)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2)] >> (bitDepth - BIT8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2) + 1] >> (bitDepth - BIT8)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2) + 3] >> (bitDepth - BIT8)); // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2) - 1] >> (bitDepth - BIT8)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width * 2) + (j * 2) + 1] >> (bitDepth - BIT8)); // v
                    }
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width * 2) + (j * 2)];
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(i * width * 2) + (j * 2) + 1]; // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(i * width * 2) + (j * 2) + 3]; // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(i * width * 2) + (j * 2) - 1]; // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(i * width * 2) + (j * 2) + 1]; // v
                    }
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV422_YUYV_AS1)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            uint32_t y_mask = bitDepth == BIT16 ? UINT32_MAX : (1U << (2 * bitDepth)) - 1U;
            const uint32_t uv_mask = (1U << bitDepth) - 1U;
            y_mask = y_mask - uv_mask;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j] & y_mask) >> (bitDepth + bitDepth - BIT8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j] & uv_mask) >> (bitDepth - BIT8)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j + 1] & uv_mask) >> (bitDepth - BIT8)); // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j - 1] & uv_mask) >> (bitDepth - BIT8)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j] & uv_mask) >> (bitDepth - BIT8)); // v
                    }
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            uint16_t y_mask = bitDepth == BIT8 ? UINT16_MAX : (1U << (2 * bitDepth)) - 1U;
            const uint16_t uv_mask = (1U << bitDepth) - 1U;
            y_mask = y_mask - uv_mask;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    const uint16_t x_r = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width) + j] & y_mask) >> bitDepth);
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(x_r << (BIT8 - bitDepth));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width) + j] & uv_mask) << (BIT8 - bitDepth)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width) + j + 1] & uv_mask) << (BIT8 - bitDepth)); // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width) + j - 1] & uv_mask) << (BIT8 - bitDepth)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width) + j] & uv_mask) << (BIT8 - bitDepth)); // v
                    }
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV422_UYVY_AS1)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            uint32_t uv_mask = bitDepth == BIT16 ? UINT32_MAX : (1U << (2 * bitDepth)) - 1U;
            const uint32_t y_mask = (1U << bitDepth) - 1U;
            uv_mask = uv_mask - y_mask;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j] & y_mask) >> (bitDepth - BIT8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j] & uv_mask) >> (bitDepth + bitDepth - BIT8));     // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j + 1] & uv_mask) >> (bitDepth + bitDepth - BIT8)); // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j - 1] & uv_mask) >> (bitDepth + bitDepth - BIT8)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[(i * width) + j] & uv_mask) >> (bitDepth + bitDepth - BIT8));     // v
                    }
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            uint16_t uv_mask = bitDepth == BIT8 ? UINT16_MAX : (1U << (2 * bitDepth)) - 1U;
            const uint16_t y_mask = (1U << bitDepth) - 1U;
            uv_mask = uv_mask - y_mask;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[(i * width) + j] & y_mask) << (BIT8 - bitDepth));
                    if (j % 2 == 0)
                    {
                        uint16_t g_x = (((uint16_t *)yuvBuf)[(i * width) + j] & uv_mask) >> bitDepth;
                        g_x = g_x << (BIT8 - bitDepth);
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(g_x);         // u

                        uint16_t b_x = (((uint16_t *)yuvBuf)[(i * width) + j + 1] & uv_mask) >> bitDepth;
                        b_x = b_x << (BIT8 - bitDepth);
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(b_x);     // v
                    }
                    else
                    {
                        uint16_t g_x = (((uint16_t *)yuvBuf)[(i * width) + j - 1] & uv_mask) >> bitDepth;
                        g_x = g_x << (BIT8 - bitDepth);
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(g_x);     // u

                        uint16_t b_x = (((uint16_t *)yuvBuf)[(i * width) + j] & uv_mask) >> bitDepth;
                        b_x = b_x << (BIT8 - bitDepth);
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(b_x);         // v
                    }
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV420_NV12)
    { // yyyyy...uvuv
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width) + j] >> (bitDepth - BIT8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width) + j] >> (bitDepth - BIT8));     // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width) + j + 1] >> (bitDepth - BIT8)); // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width) + j - 1] >> (bitDepth - BIT8)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width) + j] >> (bitDepth - BIT8));     // v
                    }
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width) + j];
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(width * height) + ((i / 2) * width) + j];     // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(width * height) + ((i / 2) * width) + j + 1]; // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(width * height) + ((i / 2) * width) + j - 1]; // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(width * height) + ((i / 2) * width) + j];     // v
                    }
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV420_NV21)
    { // yyyyy...vuvu
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width) + j] >> (bitDepth - BIT8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width) + j + 1] >> (bitDepth - BIT8)); // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width) + j] >> (bitDepth - BIT8));     // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width) + j] >> (bitDepth - BIT8));     // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width) + j - 1] >> (bitDepth - BIT8)); // v
                    }
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width) + j];
                    if (j % 2 == 0)
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(width * height) + ((i / 2) * width) + j + 1]; // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(width * height) + ((i / 2) * width) + j];     // v
                    }
                    else
                    {
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(width * height) + ((i / 2) * width) + j];     // u
                        rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(width * height) + ((i / 2) * width) + j - 1]; // v
                    }
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV420P_YU12)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width) + j] >> (bitDepth - BIT8));
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width / 2) + (j / 2)] >> (bitDepth - BIT8));                      // u
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) +(width * height / 4) + ((i / 2) * width / 2) + (j / 2)] >> (bitDepth - BIT8)); // v
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width) + j];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(width * height) + ((i / 2) * width / 2) + (j / 2)] >> (bitDepth - BIT8);                      // u
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(width * height) + (width * height / 4) + ((i / 2) * width / 2) + (j / 2)] >> (bitDepth - BIT8); // v
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV420P_YV12)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width) + j] >> (bitDepth - BIT8));
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + ((i / 2) * width / 2) + (j / 2)] >> (bitDepth - BIT8));                      // v
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(width * height) + (width * height / 4) + ((i / 2) * width / 2) + (j / 2)] >> (bitDepth - BIT8)); // u
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width) + j];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = yuvBuf[(width * height) + ((i / 2) * width / 2) + (j / 2)] >> (bitDepth - BIT8);                      // v
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = yuvBuf[(width * height) + (width * height / 4) + ((i / 2) * width / 2) + (j / 2)] >> (bitDepth - BIT8); // u
                }
            }
        }
    }
    else if (yuv_tp == YuvType::YUV400)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[(i * width) + j] >> (bitDepth - BIT8));
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0];
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = yuvBuf[(i * width) + j];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0];
                    rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0];
                }
            }
        }
    }

    if (yuv_tp != YuvType::YUV400)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                const short y_val = rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0];
                const short cb_v = static_cast<short>((int)rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] - 128);
                const short cr_v = static_cast<short>((int)rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] - 128);
                const short r_val = static_cast<short>((float)y_val + (1.403F * (float)cr_v)); // NOLINT(readability-function-cognitive-complexity)
                const short g_val = static_cast<short>((float)y_val - (0.714F * (float)cr_v) - (0.344F * (float)cb_v)); // NOLINT(readability-function-cognitive-complexity)
                const short b_val = static_cast<short>((float)y_val + (1.773F * (float)cb_v)); // NOLINT(readability-function-cognitive-complexity)

                rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 0] = static_cast<unsigned char>(CLIP3(r_val, 0, 255));
                rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 1] = static_cast<unsigned char>(CLIP3(g_val, 0, 255));
                rgb888Buf[(i * wholepixperline * 3) + (j * 3) + 2] = static_cast<unsigned char>(CLIP3(b_val, 0, 255));
            }
        }
    }
}

void convertYUV2RGB888(const unsigned char *yBuf, const unsigned char *uBuf, const unsigned char *vBuf, unsigned char *rgb888Buf, // NOLINT(readability-function-cognitive-complexity)
    int bitDepth, int y_stride, int u_stride, int v_stride, int width, int height, YuvRatioType yuv_tp, int rgb_line_stride)
{
    if (yuv_tp == YuvRatioType::UV444)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = static_cast<unsigned char>((((uint16_t *)yBuf)[(i * y_stride) + j]) >> (bitDepth - BIT8));
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = static_cast<unsigned char>((((uint16_t *)uBuf)[(i * u_stride) + j]) >> (bitDepth - BIT8));
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = static_cast<unsigned char>((((uint16_t *)vBuf)[(i * v_stride) + j]) >> (bitDepth - BIT8));
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = yBuf[(i * y_stride) + j];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = uBuf[(i * u_stride) + j];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = vBuf[(i * v_stride) + j];
                }
            }
        }
    }
    else if (yuv_tp == YuvRatioType::UV422)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yBuf)[(i * y_stride) + j] >> (bitDepth - BIT8));
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)uBuf)[(i * u_stride) + (j / 2)] >> (bitDepth - BIT8));
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)vBuf)[(i * v_stride) + (j / 2)] >> (bitDepth - BIT8));
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = yBuf[(i * y_stride) + j];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = uBuf[(i * u_stride) + (j / 2)];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = vBuf[(i * v_stride) + (j / 2)];
                }
            }
        }
    }
    else if (yuv_tp == YuvRatioType::UV420)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yBuf)[(i * y_stride) + j] >> (bitDepth - BIT8));
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = static_cast<unsigned char>(((uint16_t *)uBuf)[((i / 2) * u_stride) + (j / 2)] >> (bitDepth - BIT8));
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = static_cast<unsigned char>(((uint16_t *)vBuf)[((i / 2) * v_stride) + (j / 2)] >> (bitDepth - BIT8));
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = yBuf[(i * y_stride) + j];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = uBuf[((i / 2) * u_stride) + (j / 2)];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = vBuf[((i / 2) * v_stride) + (j / 2)];
                }
            }
        }
    }
    else if (yuv_tp == YuvRatioType::UV400)
    {
        if (bitDepth > BIT8 && bitDepth <= BIT16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = static_cast<unsigned char>(((uint16_t *)yBuf)[(i * y_stride) + j] >> (bitDepth - BIT8));
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0];
                }
            }
        }
        else if (bitDepth <= BIT8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = yBuf[(i * y_stride) + j];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0];
                    rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0];
                }
            }
        }
    }

    if (yuv_tp != YuvRatioType::UV400)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                const short y_val = (short)rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0];
                const short cb_v = static_cast<short>((int)(rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1]) - 128);
                const short cr_v = static_cast<short>((int)(rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2]) - 128);
                const short r_val = static_cast<short>((float)y_val + (1.403F * (float)cr_v)); // NOLINT(readability-function-cognitive-complexity)
                const short g_val = static_cast<short>((float)y_val - (0.714F * (float)cr_v) - (0.344F * (float)cb_v)); // NOLINT(readability-function-cognitive-complexity)
                const short b_val = static_cast<short>((float)y_val + (1.773F * (float)cb_v)); // NOLINT(readability-function-cognitive-complexity)

                rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 0] = static_cast<unsigned char>(CLIP3(r_val, 0, 255));
                rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 1] = static_cast<unsigned char>(CLIP3(g_val, 0, 255));
                rgb888Buf[(i * rgb_line_stride * 3) + (j * 3) + 2] = static_cast<unsigned char>(CLIP3(b_val, 0, 255));
            }
        }
    }
}