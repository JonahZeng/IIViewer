#include "common_type.h"

void convertYUV2RGB888(const unsigned char *yuvBuf, unsigned char *rgb888Buf, int bitDepth, int width, int height, YuvType tp, int wholepixperline);
void convertYUV2RGB888(const unsigned char *yBuf, const unsigned char *uBuf, const unsigned char *vBuf, unsigned char *rgb888Buf, 
    int bitDepth, int y_stride, int u_stride, int v_stride, int width, int height, YuvRatioType tp, int rgb_line_stride);