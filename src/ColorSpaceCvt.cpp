#include "ColorSpaceCvt.h"
#include <cstdint>

void convertYUV2RGB888(unsigned char *yuvBuf, unsigned char *rgb888Buf, int bitDepth, int width, int height, YuvType tp, int wholepixperline)
{
    if (tp == YuvType::YUV444_INTERLEAVE)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width * 3 + j * 3 + 0]) >> (bitDepth - 8));
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width * 3 + j * 3 + 1]) >> (bitDepth - 8));
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width * 3 + j * 3 + 2]) >> (bitDepth - 8));
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width * 3 + j * 3 + 0];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[i * width * 3 + j * 3 + 1];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[i * width * 3 + j * 3 + 2];
                }
            }
        }
    }
    else if (tp == YuvType::YUV444_PLANAR)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8));
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + i * width + j] >> (bitDepth - 8));
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height * 2 + i * width + j] >> (bitDepth - 8));
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width + j];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[width * height + i * width + j];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[width * height * 2 + i * width + j];
                }
            }
        }
    }
    else if (tp == YuvType::YUV422_UYVY)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 1] >> (bitDepth - 8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2] >> (bitDepth - 8));     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 2] >> (bitDepth - 8)); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2 - 2] >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2] >> (bitDepth - 8));     // v
                    }
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width * 2 + j * 2 + 1];
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[i * width * 2 + j * 2];     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[i * width * 2 + j * 2 + 2]; // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[i * width * 2 + j * 2 - 2]; // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[i * width * 2 + j * 2];     // v
                    }
                }
            }
        }
    }
    else if (tp == YuvType::YUV422_YUYV)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2] >> (bitDepth - 8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 1] >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 3] >> (bitDepth - 8)); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2 - 1] >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 1] >> (bitDepth - 8)); // v
                    }
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width * 2 + j * 2];
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[i * width * 2 + j * 2 + 1]; // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[i * width * 2 + j * 2 + 3]; // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[i * width * 2 + j * 2 - 1]; // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[i * width * 2 + j * 2 + 1]; // v
                    }
                }
            }
        }
    }
    else if (tp == YuvType::YUV422_YUYV_AS1)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            uint32_t y_mask = bitDepth == 16 ? UINT32_MAX : (1U << (2 * bitDepth)) - 1U;
            uint32_t uv_mask = (1U << bitDepth) - 1U;
            y_mask = y_mask - uv_mask;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j] & y_mask) >> (bitDepth + bitDepth - 8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j] & uv_mask) >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j + 1] & uv_mask) >> (bitDepth - 8)); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j - 1] & uv_mask) >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j] & uv_mask) >> (bitDepth - 8)); // v
                    }
                }
            }
        }
        else if (bitDepth <= 8)
        {
            uint16_t y_mask = bitDepth == 8 ? UINT16_MAX : (1U << (2 * bitDepth)) - 1U;
            uint16_t uv_mask = (1U << bitDepth) - 1U;
            y_mask = y_mask - uv_mask;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j] & y_mask) >> (bitDepth + bitDepth - 8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j] & uv_mask) >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j + 1] & uv_mask) >> (bitDepth - 8)); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j - 1] & uv_mask) >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j] & uv_mask) >> (bitDepth - 8)); // v
                    }
                }
            }
        }
    }
    else if (tp == YuvType::YUV422_UYVY_AS1)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            uint32_t uv_mask = bitDepth == 16 ? UINT32_MAX : (1U << (2 * bitDepth)) - 1U;
            uint32_t y_mask = (1U << bitDepth) - 1U;
            uv_mask = uv_mask - y_mask;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j] & y_mask) >> (bitDepth - 8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j] & uv_mask) >> (bitDepth + bitDepth - 8));     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j + 1] & uv_mask) >> (bitDepth + bitDepth - 8)); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j - 1] & uv_mask) >> (bitDepth + bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint32_t *)yuvBuf)[i * width + j] & uv_mask) >> (bitDepth + bitDepth - 8));     // v
                    }
                }
            }
        }
        else if (bitDepth <= 8)
        {
            uint16_t uv_mask = bitDepth == 8 ? UINT16_MAX : (1U << (2 * bitDepth)) - 1U;
            uint16_t y_mask = (1U << bitDepth) - 1U;
            uv_mask = uv_mask - y_mask;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j] & y_mask) >> (bitDepth - 8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j] & uv_mask) >> (bitDepth + bitDepth - 8));     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j + 1] & uv_mask) >> (bitDepth + bitDepth - 8));     // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j - 1] & uv_mask) >> (bitDepth + bitDepth - 8));     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>((((uint16_t *)yuvBuf)[i * width + j] & uv_mask) >> (bitDepth + bitDepth - 8));     // v
                    }
                }
            }
        }
    }
    else if (tp == YuvType::YUV420_NV12)
    { // yyyyy...uvuv
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j] >> (bitDepth - 8));     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j + 1] >> (bitDepth - 8)); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j - 1] >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j] >> (bitDepth - 8));     // v
                    }
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width + j];
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[width * height + (i / 2) * width + j];     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[width * height + (i / 2) * width + j + 1]; // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[width * height + (i / 2) * width + j - 1]; // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[width * height + (i / 2) * width + j];     // v
                    }
                }
            }
        }
    }
    else if (tp == YuvType::YUV420_NV21)
    { // yyyyy...vuvu
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8));
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j + 1] >> (bitDepth - 8)); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j] >> (bitDepth - 8));     // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j] >> (bitDepth - 8));     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j - 1] >> (bitDepth - 8)); // v
                    }
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width + j];
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[width * height + (i / 2) * width + j + 1]; // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[width * height + (i / 2) * width + j];     // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[width * height + (i / 2) * width + j];     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[width * height + (i / 2) * width + j - 1]; // v
                    }
                }
            }
        }
    }
    else if (tp == YuvType::YUV420P_YU12)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8));
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8));                      // u
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + width * height / 4 + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8)); // v
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width + j];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[width * height + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8);                      // u
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[width * height + width * height / 4 + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8); // v
                }
            }
        }
    }
    else if (tp == YuvType::YUV420P_YV12)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8));
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8));                      // v
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[width * height + width * height / 4 + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8)); // u
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width + j];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = yuvBuf[width * height + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8);                      // v
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = yuvBuf[width * height + width * height / 4 + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8); // u
                }
            }
        }
    }
    else if (tp == YuvType::YUV400)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8));
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = rgb888Buf[i * wholepixperline * 3 + j * 3 + 0];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = rgb888Buf[i * wholepixperline * 3 + j * 3 + 0];
                }
            }
        }
        else if (bitDepth <= 8)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = yuvBuf[i * width + j];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = rgb888Buf[i * wholepixperline * 3 + j * 3 + 0];
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = rgb888Buf[i * wholepixperline * 3 + j * 3 + 0];
                }
            }
        }
    }

    if (tp != YuvType::YUV400)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                short y = rgb888Buf[i * wholepixperline * 3 + j * 3 + 0];
                short cb = (short)(rgb888Buf[i * wholepixperline * 3 + j * 3 + 1]) - 128;
                short cr = (short)(rgb888Buf[i * wholepixperline * 3 + j * 3 + 2]) - 128;
                short r = static_cast<short>(y + 1.403f * cr);
                short g = static_cast<short>(y - 0.714f * cr - 0.344f * cb);
                short b = static_cast<short>(y + 1.773f * cb);

                rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = static_cast<unsigned char>(CLIP3(r, 0, 255));
                rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = static_cast<unsigned char>(CLIP3(g, 0, 255));
                rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = static_cast<unsigned char>(CLIP3(b, 0, 255));
            }
        }
    }
}
