#include "ImageWidget.h"
#include "RoiDataExportDlg.h"
#include <QMessageBox>
#include <cmath>

static void computeStats(const std::vector<unsigned int>& data, unsigned int& mean, unsigned long long int& variance, unsigned int& stddev)
{
    size_t n = data.size();
    if (n == 0) return;

    mean = std::accumulate(data.begin(), data.end(), 0) / (unsigned int)n;

    variance = std::accumulate(data.begin(), data.end(), 0,
        [mean](unsigned long long int acc, unsigned int x)
        {
            unsigned int k_d = x > mean ? (x - mean) : (mean - x);
            return acc + (unsigned long long int)k_d * k_d;
        }) / n;

    stddev = std::sqrt(variance);
}

void ImageWidget::exportRoiData()
{
    int roi_left = qMin(ptCodInfo.originPaintCoordinates[0].x(), ptCodInfo.originPaintCoordinates[1].x()) / ptCodInfo.originScaleRatio;
    int roi_right = qMax(ptCodInfo.originPaintCoordinates[0].x(), ptCodInfo.originPaintCoordinates[1].x()) / ptCodInfo.originScaleRatio;
    int roi_top = qMin(ptCodInfo.originPaintCoordinates[0].y(), ptCodInfo.originPaintCoordinates[1].y()) / ptCodInfo.originScaleRatio;
    int roi_bottom = qMax(ptCodInfo.originPaintCoordinates[0].y(), ptCodInfo.originPaintCoordinates[1].y()) / ptCodInfo.originScaleRatio;

    // auto msg_text = QString("%1, %2, %3, %4").arg(roi_left).arg(roi_right).arg(roi_top).arg(roi_bottom);
    // QMessageBox::information(this, "roi coordinate", msg_text, QMessageBox::StandardButton::Ok);

    int pixCnt = (roi_right - roi_left) * (roi_bottom - roi_top);
    if(pixCnt > 4096)
    {
        auto ans = QMessageBox::question(this, tr("warning"), tr("pixel in roi count > 4096, are you sure to export?"), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No);
        if(ans != QMessageBox::StandardButton::Yes)
        {
            return;
        }
    }

    QString roiPixelValStr("");
    if(openedImgType == OpenedImageType::NORMAL_IMG)
    {
        roiPixelValStr.append("[");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                if(col != roi_right - 1)
                    roiPixelValStr.append(QString("[%1, %2, %3], ").arg(pixMap->pixelColor(col, row).red()).arg(pixMap->pixelColor(col, row).green()).arg(pixMap->pixelColor(col, row).blue()));
                else
                    roiPixelValStr.append(QString("[%1, %2, %3]").arg(pixMap->pixelColor(col, row).red()).arg(pixMap->pixelColor(col, row).green()).arg(pixMap->pixelColor(col, row).blue()));
            }

            if(row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if(openedImgType == OpenedImageType::PGM_IMG)
    {
        int pgmWidth = pixMap->width();
        roiPixelValStr.append("[");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int gray = 0;
                if (pgmDataBit <= 8)
                {
                    gray = ((unsigned char *)pgmDataPtr)[row * pgmWidth + col];
                }
                else if (pgmDataBit > 8 && pgmDataBit <= 16)
                {
                    gray = ((unsigned short *)pgmDataPtr)[row * pgmWidth + col];
                }

                if(col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(gray));
                else
                    roiPixelValStr.append(QString("%1").arg(gray));
            }

            if(row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if(openedImgType == OpenedImageType::PNM_IMG)
    {
        int pnmWidth = pixMap->width();
        roiPixelValStr.append("[");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int r_ = 0;
                unsigned int g_ = 0;
                unsigned int b_ = 0;
                if (pnmDataBit <= 8)
                {
                    r_ = ((unsigned char *)pgmDataPtr)[row * pnmWidth * 3 + col * 3 + 0];
                    g_ = ((unsigned char *)pgmDataPtr)[row * pnmWidth * 3 + col * 3 + 1];
                    b_ = ((unsigned char *)pgmDataPtr)[row * pnmWidth * 3 + col * 3 + 2];
                }
                else if (pgmDataBit > 8 && pgmDataBit <= 16)
                {
                    r_ = ((unsigned short *)pgmDataPtr)[row * pnmWidth * 3 + col * 3 + 0];
                    g_ = ((unsigned short *)pgmDataPtr)[row * pnmWidth * 3 + col * 3 + 1];
                    b_ = ((unsigned short *)pgmDataPtr)[row * pnmWidth * 3 + col * 3 + 2];
                }

                if(col != roi_right - 1)
                    roiPixelValStr.append(QString("[%1, %2, %3], ").arg(r_).arg(g_).arg(b_));
                else
                    roiPixelValStr.append(QString("[%1, %2, %3]").arg(r_).arg(g_).arg(b_));
            }

            if(row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if(openedImgType == OpenedImageType::RAW_IMG)
    {
        int rawWidth = pixMap->width();

        std::vector<unsigned int> roi_r(pixCnt);
        unsigned int r_cnt = 0;
        std::vector<unsigned int> roi_g(pixCnt);
        unsigned int g_cnt = 0;
        std::vector<unsigned int> roi_b(pixCnt);
        unsigned int b_cnt = 0;
        std::vector<unsigned int> roi_ir(pixCnt);
        unsigned int ir_cnt = 0;
        std::vector<unsigned int> roi_y(pixCnt);
        unsigned int y_cnt = 0;

        roiPixelValStr.append("[");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int gray = 0;
                if (rawDataBit <= 8)
                {
                    gray = ((unsigned char *)rawDataPtr)[row * rawWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    gray = ((unsigned short *)rawDataPtr)[row * rawWidth + col];
                }
                else if (rawDataBit > 16 && rawDataBit <= 24)
                {
                    gray = ((unsigned int *)rawDataPtr)[row * rawWidth + col];
                }

                if(col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(gray));
                else
                    roiPixelValStr.append(QString("%1").arg(gray));
                
                auto pix_by = getPixType(row, col, rawBayerType);
                if(pix_by == RawFileInfoDlg::BayerPixelType::PIX_R)
                {
                    roi_r[r_cnt] = gray;
                    r_cnt += 1;
                }
                else if(pix_by == RawFileInfoDlg::BayerPixelType::PIX_GR || pix_by == RawFileInfoDlg::BayerPixelType::PIX_GB)
                {
                    roi_g[g_cnt] = gray;
                    g_cnt += 1;
                }
                else if(pix_by == RawFileInfoDlg::BayerPixelType::PIX_B)
                {
                    roi_b[b_cnt] = gray;
                    b_cnt += 1;
                }
                else if(pix_by == RawFileInfoDlg::BayerPixelType::PIX_IR)
                {
                    roi_ir[ir_cnt] = gray;
                    ir_cnt += 1;
                }
                else if(pix_by == RawFileInfoDlg::BayerPixelType::PIX_Y)
                {
                    roi_y[y_cnt] = gray;
                    y_cnt += 1;
                }
            }

            if(row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");
        if(rawBayerType >= RawFileInfoDlg::BayerPatternType::RGGB && rawBayerType <= RawFileInfoDlg::BayerPatternType::BGGR) // 2x2 bayer
        {
            unsigned int roi_r_mean = 0, roi_r_stddev = 0;
            unsigned long long int roi_r_var = 0;
            computeStats(std::vector(roi_r.begin(), roi_r.begin() + r_cnt), roi_r_mean, roi_r_var, roi_r_stddev);

            unsigned int roi_g_mean = 0, roi_g_stddev = 0;
            unsigned long long int roi_g_var = 0;
            computeStats(std::vector(roi_g.begin(), roi_g.begin() + g_cnt), roi_g_mean, roi_g_var, roi_g_stddev);

            unsigned int roi_b_mean = 0, roi_b_stddev = 0;
            unsigned long long int roi_b_var = 0;
            computeStats(std::vector(roi_b.begin(), roi_b.begin() + b_cnt), roi_b_mean, roi_b_var, roi_b_stddev);

            roiPixelValStr.append(QString("R mean=%1, var=%2, std_dev=%3\n").arg(roi_r_mean).arg(roi_r_var).arg(roi_r_stddev));
            roiPixelValStr.append(QString("G mean=%1, var=%2, std_dev=%3\n").arg(roi_g_mean).arg(roi_g_var).arg(roi_g_stddev));
            roiPixelValStr.append(QString("B mean=%1, var=%2, std_dev=%3\n").arg(roi_b_mean).arg(roi_b_var).arg(roi_b_stddev));
        }
        else if(rawBayerType >= RawFileInfoDlg::BayerPatternType::RGGIR && rawBayerType <= RawFileInfoDlg::BayerPatternType::IRGGB) // rgbir 4x4
        {
            unsigned int roi_r_mean = 0, roi_r_stddev = 0;
            unsigned long long int roi_r_var = 0;
            computeStats(std::vector(roi_r.begin(), roi_r.begin() + r_cnt), roi_r_mean, roi_r_var, roi_r_stddev);

            unsigned int roi_g_mean = 0, roi_g_stddev = 0;
            unsigned long long int roi_g_var = 0;
            computeStats(std::vector(roi_g.begin(), roi_g.begin() + g_cnt), roi_g_mean, roi_g_var, roi_g_stddev);

            unsigned int roi_b_mean = 0, roi_b_stddev = 0;
            unsigned long long int roi_b_var = 0;
            computeStats(std::vector(roi_b.begin(), roi_b.begin() + b_cnt), roi_b_mean, roi_b_var, roi_b_stddev);

            unsigned int roi_ir_mean = 0, roi_ir_stddev = 0;
            unsigned long long int roi_ir_var = 0;
            computeStats(std::vector(roi_ir.begin(), roi_ir.begin() + ir_cnt), roi_ir_mean, roi_ir_var, roi_ir_stddev);

            roiPixelValStr.append(QString("R mean=%1, var=%2, std_dev=%3\n").arg(roi_r_mean).arg(roi_r_var).arg(roi_r_stddev));
            roiPixelValStr.append(QString("G mean=%1, var=%2, std_dev=%3\n").arg(roi_g_mean).arg(roi_g_var).arg(roi_g_stddev));
            roiPixelValStr.append(QString("B mean=%1, var=%2, std_dev=%3\n").arg(roi_b_mean).arg(roi_b_var).arg(roi_b_stddev));
            roiPixelValStr.append(QString("IR mean=%1, var=%2, std_dev=%3\n").arg(roi_ir_mean).arg(roi_ir_var).arg(roi_ir_stddev));
        }
        else // mono
        {
            unsigned int roi_y_mean = 0, roi_y_stddev = 0;
            unsigned long long int roi_y_var = 0;
            computeStats(std::vector(roi_y.begin(), roi_y.begin() + y_cnt), roi_y_mean, roi_y_var, roi_y_stddev);
            roiPixelValStr.append(QString("Y mean=%1, var=%2, std_dev=%3\n").arg(roi_y_mean).arg(roi_y_var).arg(roi_y_stddev));
        }

    }
    else if(openedImgType == OpenedImageType::YUV_IMG)
    {
        exportRoiYuvData(roiPixelValStr, roi_top, roi_bottom, roi_left, roi_right);
    }

    RoiDataExportDlg detailDlg(this);
    detailDlg.setRoiExportText(roiPixelValStr);
    detailDlg.exec();
}

void ImageWidget::exportRoiYuvData(QString &roiPixelValStr, int roi_top, int roi_bottom, int roi_left, int roi_right)
{
    int yWidth = pixMap->width();
    int yHeight = pixMap->height();

    if (yuvType == YuvFileInfoDlg::YuvType::YUV444_INTERLEAVE)
    {
        roiPixelValStr.append("Y: [");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 3 + col * 3 + 0];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 3 + col * 3 + 0];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("U: [");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int u_ = 0;
                if (yuvDataBit <= 8)
                {
                    u_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 3 + col * 3 + 1];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    u_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 3 + col * 3 + 1];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(u_));
                else
                    roiPixelValStr.append(QString("%1").arg(u_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("V: [");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int v_ = 0;
                if (yuvDataBit <= 8)
                {
                    v_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 3 + col * 3 + 2];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    v_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 3 + col * 3 + 2];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(v_));
                else
                    roiPixelValStr.append(QString("%1").arg(v_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if (yuvType == YuvFileInfoDlg::YuvType::YUV444_PLANAR)
    {
        roiPixelValStr.append("Y: [");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("U: [");
        unsigned int u_oft = yWidth * yHeight;
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int u_ = 0;
                if (yuvDataBit <= 8)
                {
                    u_ = ((unsigned char *)yuvDataPtr)[u_oft + row * yWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    u_ = ((unsigned short *)yuvDataPtr)[u_oft + row * yWidth + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(u_));
                else
                    roiPixelValStr.append(QString("%1").arg(u_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("V: [");
        unsigned int v_oft = yWidth * yHeight * 2;
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int v_ = 0;
                if (yuvDataBit <= 8)
                {
                    v_ = ((unsigned char *)yuvDataPtr)[v_oft + row * yWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    v_ = ((unsigned short *)yuvDataPtr)[v_oft + row * yWidth + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(v_));
                else
                    roiPixelValStr.append(QString("%1").arg(v_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if (yuvType == YuvFileInfoDlg::YuvType::YUV422_UYVY)
    {
        roiPixelValStr.append("Y: [");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 2 + col * 2 + 1];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 2 + col * 2 + 1];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("U: [");
        // UYVY....UYVY....
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1: roi_left; col < roi_right; col += 2)
            {
                unsigned int u_ = 0;
                if (yuvDataBit <= 8)
                {
                    u_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 2 + col * 2];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    u_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 2 + col * 2];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(u_));
                else
                    roiPixelValStr.append(QString("%1").arg(u_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("V: [");
        // UYVY....UYVY....
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left : roi_left + 1; col < roi_right; col += 2)
            {
                unsigned int v_ = 0;
                if (yuvDataBit <= 8)
                {
                    v_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 2 + col * 2];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    v_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 2 + col * 2];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(v_));
                else
                    roiPixelValStr.append(QString("%1").arg(v_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if (yuvType == YuvFileInfoDlg::YuvType::YUV422_YUYV)
    {
        roiPixelValStr.append("Y: [");
        // YUYV....YUYV....
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 2 + col * 2];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 2 + col * 2];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("U: [");
        // YUYV....YUYV....
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1: roi_left; col < roi_right; col += 2)
            {
                unsigned int u_ = 0;
                if (yuvDataBit <= 8)
                {
                    u_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 2 + col * 2 + 1];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    u_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 2 + col * 2 + 1];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(u_));
                else
                    roiPixelValStr.append(QString("%1").arg(u_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("V: [");
        // YUYV....YUYV....
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left : roi_left + 1; col < roi_right; col += 2)
            {
                unsigned int v_ = 0;
                if (yuvDataBit <= 8)
                {
                    v_ = ((unsigned char *)yuvDataPtr)[row * yWidth * 2 + col * 2 + 1];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    v_ = ((unsigned short *)yuvDataPtr)[row * yWidth * 2 + col * 2 + 1];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(v_));
                else
                    roiPixelValStr.append(QString("%1").arg(v_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if (yuvType == YuvFileInfoDlg::YuvType::YUV420_NV12)
    {
        roiPixelValStr.append("Y: [");
        // YYYYYY
        // UVUVUV
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("U: [");
        // YYYYYY
        // UVUVUV
        unsigned int y_oft = yWidth * yHeight;
        for (int row = (roi_top & 0x1) ? roi_top + 1 : roi_top; row < roi_bottom; row += 2)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1 : roi_left; col < roi_right; col += 2)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[y_oft + row * yWidth / 2 + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[y_oft + row * yWidth / 2 + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("V: [");
        // YYYYYY
        // UVUVUV
        for (int row = (roi_top & 0x1) ? roi_top + 1 : roi_top; row < roi_bottom; row += 2)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1 : roi_left; col < roi_right; col += 2)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[y_oft + row * yWidth / 2 + col + 1];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[y_oft + row * yWidth / 2 + col + 1];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if (yuvType == YuvFileInfoDlg::YuvType::YUV420_NV21)
    {
        roiPixelValStr.append("Y: [");
        // YYYYYY
        // VUVUVU
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("U: [");
        // YYYYYY
        // VUVUVU
        unsigned int y_oft = yWidth * yHeight;
        for (int row = (roi_top & 0x1) ? roi_top + 1 : roi_top; row < roi_bottom; row += 2)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1 : roi_left; col < roi_right; col += 2)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[y_oft + row * yWidth / 2 + col + 1];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[y_oft + row * yWidth / 2 + col + 1];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("V: [");
        // YYYYYY
        // VUVUVU
        for (int row = (roi_top & 0x1) ? roi_top + 1 : roi_top; row < roi_bottom; row += 2)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1 : roi_left; col < roi_right; col += 2)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[y_oft + row * yWidth / 2 + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[y_oft + row * yWidth / 2 + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if (yuvType == YuvFileInfoDlg::YuvType::YUV420P_YU12)
    {
        roiPixelValStr.append("Y: [");
        // YYYYYY
        // UUU
        // VVV
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("U: [");
        // YYYYYY
        // UUU
        // VVV
        unsigned int y_oft = yWidth * yHeight;
        for (int row = (roi_top & 0x1) ? roi_top + 1 : roi_top; row < roi_bottom; row += 2)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1 : roi_left; col < roi_right; col += 2)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[y_oft + row * yWidth / 4 + col / 2];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[y_oft + row * yWidth / 4 + col / 2];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("V: [");
        // YYYYYY
        // UUU
        // VVV
        unsigned int u_oft = yWidth * yHeight + yWidth * yHeight / 4;
        for (int row = (roi_top & 0x1) ? roi_top + 1 : roi_top; row < roi_bottom; row += 2)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1 : roi_left; col < roi_right; col += 2)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[u_oft + row * yWidth / 4 + col / 2];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[u_oft + row * yWidth / 4 + col / 2];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if (yuvType == YuvFileInfoDlg::YuvType::YUV420P_YV12)
    {
        roiPixelValStr.append("Y: [");
        // YYYYYY
        // VVV
        // UUU
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("U: [");
        // YYYYYY
        // VVV
        // UUU
        unsigned int u_oft = yWidth * yHeight + yWidth * yHeight / 4;
        for (int row = (roi_top & 0x1) ? roi_top + 1 : roi_top; row < roi_bottom; row += 2)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1 : roi_left; col < roi_right; col += 2)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[u_oft + row * yWidth / 4 + col / 2];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[u_oft + row * yWidth / 4 + col / 2];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]\n");

        roiPixelValStr.append("V: [");
        // YYYYYY
        // VVV
        // UUU
        unsigned int y_oft = yWidth * yHeight;
        for (int row = (roi_top & 0x1) ? roi_top + 1 : roi_top; row < roi_bottom; row += 2)
        {
            roiPixelValStr.append("[");
            for (int col = (roi_left & 0x1) ? roi_left + 1 : roi_left; col < roi_right; col += 2)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[y_oft + row * yWidth / 4 + col / 2];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[y_oft + row * yWidth / 4 + col / 2];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
    else if (yuvType == YuvFileInfoDlg::YuvType::YUV400)
    {
        roiPixelValStr.append("[");
        for (int row = roi_top; row < roi_bottom; row++)
        {
            roiPixelValStr.append("[");
            for (int col = roi_left; col < roi_right; col++)
            {
                unsigned int y_ = 0;
                if (yuvDataBit <= 8)
                {
                    y_ = ((unsigned char *)yuvDataPtr)[row * yWidth + col];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    y_ = ((unsigned short *)yuvDataPtr)[row * yWidth + col];
                }
                if (col != roi_right - 1)
                    roiPixelValStr.append(QString("%1, ").arg(y_));
                else
                    roiPixelValStr.append(QString("%1").arg(y_));
            }

            if (row != roi_bottom - 1)
            {
                roiPixelValStr.append("]\n");
            }
            else
            {
                roiPixelValStr.append("]");
            }
        }
        roiPixelValStr.append("]");
    }
}
