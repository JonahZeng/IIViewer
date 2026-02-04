#include "ImgInfoDlg.h"
#include "IIPviewer.h"
#include "ImgDiffReportDlg.h"
#include <QMessageBox>

void IIPviewer::updateExchangeBtn()
{
    if (ui.imageLabel[0]->openedImgType != UNKNOW_IMG && ui.imageLabel[1]->openedImgType != UNKNOW_IMG)
    {
        ui.exchangeAreaPreviewBtn->setEnabled(true);
        ui.imageDiffInfoBtn->setEnabled(true);
    }
    else
    {
        ui.exchangeAreaPreviewBtn->setEnabled(false);
        ui.imageDiffInfoBtn->setEnabled(false);
    }


    ui.imageInfoBtn->setEnabled((ui.imageLabel[0]->openedImgType != UNKNOW_IMG) || (ui.imageLabel[1]->openedImgType != UNKNOW_IMG));
    
}

void IIPviewer::updateZoomLabelText()
{
    if (ui.imageLabel[0]->openedImgType == UNKNOW_IMG && ui.imageLabel[1]->openedImgType == UNKNOW_IMG)
    {
        ui.zoomRatioLabel->setEnabled(false);
        ui.zoomRatioLabel->setText("1x");
    }
    else
    {
        ui.zoomRatioLabel->setEnabled(true);
    }
}

void IIPviewer::exchangeRight2LeftImg()
{
    ui.imageLabel[0]->acceptImgFromOther(ui.imageLabel[1]);
    ui.exchangeAreaPreviewBtn->setIcon(QIcon(":/image/src/resource/right2left-pressed_w20.png"));
}

void IIPviewer::restoreLeftImg()
{
    ui.imageLabel[0]->restoreImg();
    ui.exchangeAreaPreviewBtn->setIcon(QIcon(":/image/src/resource/right2left_w20.png"));
}

void IIPviewer::showImageInfo()
{
    ImgInfoDlg dlg(this);
    if (openedFile[0].endsWith("raw", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], ui.imageLabel[0]->rawBayerType, YUV_UNKNOW, ui.imageLabel[0]->rawDataBit, true);
    }
    else if (openedFile[0].endsWith("pnm", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], BAYER_UNKNOW, YUV_UNKNOW, ui.imageLabel[0]->pnmDataBit, true);
    }
    else if (openedFile[0].endsWith("pgm", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], BAYER_UNKNOW, YUV_UNKNOW, ui.imageLabel[0]->pgmDataBit, true);
    }
    else if (openedFile[0].endsWith("yuv", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], BAYER_UNKNOW, ui.imageLabel[0]->yuvType, ui.imageLabel[0]->yuvDataBit, true);
    }
    else if (openedFile[0].endsWith("jpg", Qt::CaseSensitivity::CaseInsensitive) || openedFile[0].endsWith("jpeg", Qt::CaseSensitivity::CaseInsensitive) || 
        openedFile[0].endsWith("bmp", Qt::CaseSensitivity::CaseInsensitive) || openedFile[0].endsWith("png", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], BAYER_UNKNOW, YUV_UNKNOW, 8, true);
    }

    if (openedFile[1].endsWith("raw", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], ui.imageLabel[1]->rawBayerType, YUV_UNKNOW, ui.imageLabel[1]->rawDataBit, false);
    }
    else if (openedFile[1].endsWith("pnm", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], BAYER_UNKNOW, YUV_UNKNOW, ui.imageLabel[1]->pnmDataBit, false);
    }
    else if (openedFile[1].endsWith("pgm", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], BAYER_UNKNOW, YUV_UNKNOW, ui.imageLabel[1]->pgmDataBit, false);
    }
    else if (openedFile[1].endsWith("yuv", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], BAYER_UNKNOW, ui.imageLabel[1]->yuvType, ui.imageLabel[1]->yuvDataBit, false);
    }
    else if (openedFile[1].endsWith("jpg", Qt::CaseSensitivity::CaseInsensitive) || openedFile[1].endsWith("jpeg", Qt::CaseSensitivity::CaseInsensitive) || 
        openedFile[1].endsWith("bmp", Qt::CaseSensitivity::CaseInsensitive) || openedFile[1].endsWith("png", Qt::CaseSensitivity::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], BAYER_UNKNOW, YUV_UNKNOW, 8, false);
    }
    dlg.exec();
}

static QString compareNormalImage(const QImage* left, const QImage* right)
{
    Q_ASSERT(left->size() == right->size());
    QSize imgSize = left->size();

    quint64 pixDiffSum = 0;
    quint32 pixDiff = 0;
    quint32 pixMaxDiff = 0;
    quint32 pixMinDiff = 3 * 255;
    QPoint maxDiffPos(-1, -1);
    for (int y = 0; y < imgSize.height(); y++)
    {
        for (int x = 0; x < imgSize.width(); x++)
        {
            unsigned int l_red = qRed(left->pixel(x, y));
            unsigned int r_red = qRed(right->pixel(x, y));
            unsigned int l_green = qGreen(left->pixel(x, y));
            unsigned int r_green = qGreen(right->pixel(x, y));
            unsigned int l_blue = qBlue(left->pixel(x, y));
            unsigned int r_blue = qBlue(right->pixel(x, y));

            pixDiff = l_red > r_red ? l_red - r_red : r_red - l_red;
            pixDiff += l_green > r_green ? l_green - r_green : r_green - l_green;
            pixDiff += l_blue > r_blue ? l_blue - r_blue : r_blue - l_blue;

            if (pixDiff > pixMaxDiff)
            {
                pixMaxDiff = pixDiff;
                maxDiffPos.setX(x);
                maxDiffPos.setY(y);
            }
            if (pixDiff < pixMinDiff)
            {
                pixMinDiff = pixDiff;
            }
            pixDiffSum += pixDiff;
        }
    }
    int pixCnt = imgSize.height() * imgSize.width();
    auto tmp = QString("max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixMaxDiff).arg(pixMinDiff).arg(pixDiffSum / (qreal)pixCnt).arg(maxDiffPos.y()).arg(maxDiffPos.x());
    return tmp;
}

static QString comparePgmImage(const unsigned char* left, const unsigned char* right, const int bits, const QSize imgSize)
{
    const int width = imgSize.width();
    quint64 pixDiffSum = 0;
    quint32 pixDiff = 0;
    quint32 pixMaxDiff = 0;
    quint32 pixMinDiff = (1u << bits) - 1;
    QPoint maxDiffPos(-1, -1);
    for (int y = 0; y < imgSize.height(); y++)
    {
        for (int x = 0; x < imgSize.width(); x++)
        {
            unsigned int l_gray = bits <= 8 ? left[y * width + x] : ((quint16*)left)[y * width + x];
            unsigned int r_gray = bits <= 8 ? right[y * width + x] : ((quint16*)right)[y * width + x];
            pixDiff = l_gray > r_gray ? l_gray - r_gray : r_gray - l_gray;

            if (pixDiff > pixMaxDiff)
            {
                pixMaxDiff = pixDiff;
                maxDiffPos.setX(x);
                maxDiffPos.setY(y);
            }
            if (pixDiff < pixMinDiff)
            {
                pixMinDiff = pixDiff;
            }
            pixDiffSum += pixDiff;
        }
    }
    int pixCnt = imgSize.height() * imgSize.width();
    auto tmp = QString("max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixMaxDiff).arg(pixMinDiff).arg(pixDiffSum / (qreal)pixCnt).arg(maxDiffPos.y()).arg(maxDiffPos.x());
    return tmp;
}

static QString comparePnmImage(const unsigned char* left, const unsigned char* right, const int bits, const QSize imgSize)
{
    const int width = imgSize.width();
    quint64 pixDiffSum = 0;
    quint32 pixDiff = 0;
    quint32 pixMaxDiff = 0;
    quint32 pixMinDiff = 3 * ((1u << bits) - 1);
    QPoint maxDiffPos(-1, -1);
    for (int y = 0; y < imgSize.height(); y++)
    {
        for (int x = 0; x < imgSize.width(); x++)
        {
            unsigned int l_red = bits <= 8 ? left[y * width * 3 + x * 3] : ((quint16*)left)[y * width * 3 + x * 3];
            unsigned int r_red = bits <= 8 ? right[y * width * 3 + x * 3] : ((quint16*)right)[y * width * 3 + x * 3];
            unsigned int l_green = bits <= 8 ? left[y * width * 3 + x * 3 + 1] : ((quint16*)left)[y * width * 3 + x * 3 + 1];
            unsigned int r_green = bits <= 8 ? right[y * width * 3 + x * 3 + 1] : ((quint16*)right)[y * width * 3 + x * 3 + 1];
            unsigned int l_blue = bits <= 8 ? left[y * width * 3 + x * 3 + 2] : ((quint16*)left)[y * width * 3 + x * 3 + 2];
            unsigned int r_blue = bits <= 8 ? right[y * width * 3 + x * 3 + 2] : ((quint16*)right)[y * width * 3 + x * 3 + 2];
            pixDiff = l_red > r_red ? l_red - r_red : r_red - l_red;
            pixDiff += l_green > r_green ? l_green - r_green : r_green - l_green;
            pixDiff += l_blue > r_blue ? l_blue - r_blue : r_blue - l_blue;

            if (pixDiff > pixMaxDiff)
            {
                pixMaxDiff = pixDiff;
                maxDiffPos.setX(x);
                maxDiffPos.setY(y);
            }
            if (pixDiff < pixMinDiff)
            {
                pixMinDiff = pixDiff;
            }
            pixDiffSum += pixDiff;
        }
    }
    int pixCnt = imgSize.height() * imgSize.width();
    auto tmp = QString("max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixMaxDiff).arg(pixMinDiff).arg(pixDiffSum / (qreal)pixCnt).arg(maxDiffPos.y()).arg(maxDiffPos.x());
    return tmp;
}

static QString compareRawImage(const unsigned char* left, const unsigned char* right, const int bits, const QSize imgSize)
{
    const int width = imgSize.width();
    quint64 pixDiffSum = 0;
    quint32 pixDiff = 0;
    quint32 pixMaxDiff = 0;
    quint32 pixMinDiff = (1u << bits) - 1;
    QPoint maxDiffPos(-1, -1);
    for (int y = 0; y < imgSize.height(); y++)
    {
        for (int x = 0; x < imgSize.width(); x++)
        {
            unsigned int l_gray = bits <= 8 ? left[y * width + x] : (bits <= 16 ? ((quint16*)left)[y * width + x] : ((quint32*)left)[y * width + x]);
            unsigned int r_gray = bits <= 8 ? right[y * width + x] : (bits <= 16 ? ((quint16*)right)[y * width + x] : ((quint32*)right)[y * width + x]);
            pixDiff = l_gray > r_gray ? l_gray - r_gray : r_gray - l_gray;

            if (pixDiff > pixMaxDiff)
            {
                pixMaxDiff = pixDiff;
                maxDiffPos.setX(x);
                maxDiffPos.setY(y);
            }
            if (pixDiff < pixMinDiff)
            {
                pixMinDiff = pixDiff;
            }
            pixDiffSum += pixDiff;
        }
    }
    int pixCnt = imgSize.height() * imgSize.width();
    auto tmp = QString("max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixMaxDiff).arg(pixMinDiff).arg(pixDiffSum / (qreal)pixCnt).arg(maxDiffPos.y()).arg(maxDiffPos.x());
    return tmp;
}

static QString compareYuvImage(const unsigned char* left, const unsigned char* right, const int bits, YuvType yuvTp, const QSize imgSize)
{
    const int width = imgSize.width();
    const int height = imgSize.height();
    quint64 pixYdiffSum = 0;
    quint32 pixYdiff = 0;
    quint32 pixYmaxDiff = 0;
    quint32 pixYminDiff = (1u << bits) - 1;
    QPoint maxYdiffPos(-1, -1);

    quint64 pixUdiffSum = 0;
    quint32 pixUdiff = 0;
    quint32 pixUmaxDiff = 0;
    quint32 pixUminDiff = (1u << bits) - 1;
    QPoint maxUdiffPos(-1, -1);

    quint64 pixVdiffSum = 0;
    quint32 pixVdiff = 0;
    quint32 pixVmaxDiff = 0;
    quint32 pixVminDiff = (1u << bits) - 1;
    QPoint maxVdiffPos(-1, -1);

    if (yuvTp == YuvType::YUV400)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                unsigned int l_gray = bits <= 8 ? left[y * width + x] : ((quint16 *)left)[y * width + x];
                unsigned int r_gray = bits <= 8 ? right[y * width + x] : ((quint16 *)right)[y * width + x];
                pixYdiff = l_gray > r_gray ? l_gray - r_gray : r_gray - l_gray;

                if (pixYdiff > pixYmaxDiff)
                {
                    pixYmaxDiff = pixYdiff;
                    maxYdiffPos.setX(x);
                    maxYdiffPos.setY(y);
                }
                if (pixYdiff < pixYminDiff)
                {
                    pixYminDiff = pixYdiff;
                }
                pixYdiffSum += pixYdiff;
            }
        }
        int pixCnt = height * width;
        QString tmp = QString("Y max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixYmaxDiff).arg(pixYminDiff).arg(pixYdiffSum / (qreal)pixCnt).arg(maxYdiffPos.y()).arg(maxYdiffPos.x());
        return tmp;
    }
    else if(yuvTp == YuvType::YUV420_NV12 || yuvTp == YuvType::YUV420_NV21) // YYY...UVUV and YYY...VUVU
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                unsigned int l_gray = bits <= 8 ? left[y * width + x] : ((quint16 *)left)[y * width + x];
                unsigned int r_gray = bits <= 8 ? right[y * width + x] : ((quint16 *)right)[y * width + x];
                pixYdiff = l_gray > r_gray ? l_gray - r_gray : r_gray - l_gray;

                if (pixYdiff > pixYmaxDiff)
                {
                    pixYmaxDiff = pixYdiff;
                    maxYdiffPos.setX(x);
                    maxYdiffPos.setY(y);
                }
                if (pixYdiff < pixYminDiff)
                {
                    pixYminDiff = pixYdiff;
                }
            }
        }
        int pixCnt = height * width;
        QString tmp = QString("Y max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixYmaxDiff).arg(pixYminDiff).arg(pixYdiffSum / (qreal)pixCnt).arg(maxYdiffPos.y()).arg(maxYdiffPos.x());

        const unsigned char* l_uv_oft = left + pixCnt * (bits > 8 ? 2 : 1);
        const unsigned char* r_uv_oft = right + pixCnt * (bits > 8 ? 2 : 1);
        for (int y = 0; y < height / 2; y++)
        {
            for (int x = 0; x < width; x += 2)
            {
                unsigned int l_U = bits <= 8 ? l_uv_oft[y * width + x] : ((quint16 *)l_uv_oft)[y * width + x];
                unsigned int r_U = bits <= 8 ? r_uv_oft[y * width + x] : ((quint16 *)r_uv_oft)[y * width + x];
                pixUdiff = l_U > r_U ? l_U - r_U : r_U - l_U;

                if (pixUdiff > pixUmaxDiff)
                {
                    pixUmaxDiff = pixUdiff;
                    maxUdiffPos.setX(x);
                    maxUdiffPos.setY(y * 2);
                }
                if (pixUdiff < pixUminDiff)
                {
                    pixUminDiff = pixUdiff;
                }
                pixUdiffSum += pixUdiff;
            }
        }
        if(yuvTp == YuvType::YUV420_NV12)
            tmp += QString("U max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixUmaxDiff).arg(pixUminDiff).arg(pixUdiffSum / (qreal)(pixCnt / 4)).arg(maxUdiffPos.y()).arg(maxUdiffPos.x());
        else
            tmp += QString("V max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixUmaxDiff).arg(pixUminDiff).arg(pixUdiffSum / (qreal)(pixCnt / 4)).arg(maxUdiffPos.y()).arg(maxUdiffPos.x());

        for (int y = 0; y < height / 2; y++)
        {
            for (int x = 1; x < width; x += 2)
            {
                unsigned int l_V = bits <= 8 ? l_uv_oft[y * width + x] : ((quint16 *)l_uv_oft)[y * width + x];
                unsigned int r_V = bits <= 8 ? r_uv_oft[y * width + x] : ((quint16 *)r_uv_oft)[y * width + x];
                pixVdiff = l_V > r_V ? l_V - r_V : r_V - l_V;

                if (pixVdiff > pixVmaxDiff)
                {
                    pixVmaxDiff = pixVdiff;
                    maxVdiffPos.setX(x);
                    maxVdiffPos.setY(y * 2);
                }
                if (pixVdiff < pixVminDiff)
                {
                    pixVminDiff = pixVdiff;
                }
                pixVdiffSum += pixVdiff;
            }
        }
        if(yuvTp == YuvType::YUV420_NV12)
            tmp += QString("V max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixVmaxDiff).arg(pixVminDiff).arg(pixVdiffSum / (qreal)(pixCnt / 4)).arg(maxVdiffPos.y()).arg(maxVdiffPos.x());
        else
            tmp += QString("U max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixVmaxDiff).arg(pixVminDiff).arg(pixVdiffSum / (qreal)(pixCnt / 4)).arg(maxVdiffPos.y()).arg(maxVdiffPos.x());
        return tmp;
    }
    else if(yuvTp == YuvType::YUV420P_YU12 || yuvTp == YuvType::YUV420P_YV12) // YYY...UU..VV and YYY...VV..UU
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                unsigned int l_gray = bits <= 8 ? left[y * width + x] : ((quint16 *)left)[y * width + x];
                unsigned int r_gray = bits <= 8 ? right[y * width + x] : ((quint16 *)right)[y * width + x];
                pixYdiff = l_gray > r_gray ? l_gray - r_gray : r_gray - l_gray;

                if (pixYdiff > pixYmaxDiff)
                {
                    pixYmaxDiff = pixYdiff;
                    maxYdiffPos.setY(y);
                    maxYdiffPos.setX(x);
                }
                if (pixYdiff < pixYminDiff)
                {
                    pixYminDiff = pixYdiff;
                }
                pixYdiffSum += pixYdiff;
            }
        }
        int pixCnt = height * width;
        QString tmp = QString("Y max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixYmaxDiff).arg(pixYminDiff).arg(pixYdiffSum / (qreal)pixCnt).arg(maxYdiffPos.y()).arg(maxYdiffPos.x());

        const unsigned char* l_u_oft = left + pixCnt * (bits > 8 ? 2 : 1);
        const unsigned char* r_u_oft = right + pixCnt * (bits > 8 ? 2 : 1);
        for (int y = 0; y < height / 2; y++)
        {
            for (int x = 0; x < width / 2; x++)
            {
                unsigned int l_U = bits <= 8 ? l_u_oft[y * width / 2 + x] : ((quint16 *)l_u_oft)[y * width / 2 + x];
                unsigned int r_U = bits <= 8 ? r_u_oft[y * width / 2 + x] : ((quint16 *)r_u_oft)[y * width / 2 + x];
                pixUdiff = l_U > r_U ? l_U - r_U : r_U - l_U;

                if (pixUdiff > pixUmaxDiff)
                {
                    pixUmaxDiff = pixUdiff;
                    maxUdiffPos.setY(y * 2);
                    maxUdiffPos.setX(x * 2);
                }
                if (pixUdiff < pixUminDiff)
                {
                    pixUminDiff = pixUdiff;
                }
                pixUdiffSum += pixUdiff;
            }
        }
        if(yuvTp == YuvType::YUV420P_YU12)
            tmp += QString("U max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixUmaxDiff).arg(pixUminDiff).arg(pixUdiffSum / (qreal)(pixCnt / 4)).arg(maxUdiffPos.y()).arg(maxUdiffPos.x());
        else
            tmp += QString("V max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixUmaxDiff).arg(pixUminDiff).arg(pixUdiffSum / (qreal)(pixCnt / 4)).arg(maxUdiffPos.y()).arg(maxUdiffPos.x());

        const unsigned char* l_v_oft = l_u_oft + pixCnt * (bits > 8 ? 2 : 1) / 4;
        const unsigned char* r_v_oft = r_u_oft + pixCnt * (bits > 8 ? 2 : 1) / 4;
        for (int y = 0; y < height / 2; y++)
        {
            for (int x = 0; x < width / 2; x++)
            {
                unsigned int l_V = bits <= 8 ? l_v_oft[y * width / 2 + x] : ((quint16 *)l_v_oft)[y * width / 2 + x];
                unsigned int r_V = bits <= 8 ? r_v_oft[y * width / 2 + x] : ((quint16 *)r_v_oft)[y * width / 2 + x];
                pixVdiff = l_V > r_V ? l_V - r_V : r_V - l_V;

                if (pixVdiff > pixVmaxDiff)
                {
                    pixVmaxDiff = pixVdiff;
                    maxVdiffPos.setY(y * 2);
                    maxVdiffPos.setX(x * 2);
                }
                if (pixVdiff < pixVminDiff)
                {
                    pixVminDiff = pixVdiff;
                }
                pixVdiffSum += pixVdiff;
            }
        }
        if(yuvTp == YuvType::YUV420P_YU12)
            tmp += QString("V max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixVmaxDiff).arg(pixVminDiff).arg(pixVdiffSum / (qreal)(pixCnt / 4)).arg(maxVdiffPos.y()).arg(maxVdiffPos.x());
        else
            tmp += QString("U max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixVmaxDiff).arg(pixVminDiff).arg(pixVdiffSum / (qreal)(pixCnt / 4)).arg(maxVdiffPos.y()).arg(maxVdiffPos.x());
        return tmp;
    }
    else if(yuvTp == YuvType::YUV422_UYVY || yuvTp == YuvType::YUV422_YUYV) // UYVY and YUYV
    {
        int yx_start = yuvTp == YuvType::YUV422_UYVY ? 1 : 0;
        int ux_start = yuvTp == YuvType::YUV422_UYVY ? 0 : 1;
        int vx_start = yuvTp == YuvType::YUV422_UYVY ? 2 : 3;
        for (int y = 0; y < height; y++)
        {
            for (int x = yx_start; x < width * 2; x += 2)
            {
                unsigned int l_gray = bits <= 8 ? left[y * width * 2 + x] : ((quint16 *)left)[y * width * 2 + x];
                unsigned int r_gray = bits <= 8 ? right[y * width * 2 + x] : ((quint16 *)right)[y * width * 2 + x];
                pixYdiff = l_gray > r_gray ? l_gray - r_gray : r_gray - l_gray;

                if (pixYdiff > pixYmaxDiff)
                {
                    pixYmaxDiff = pixYdiff;
                    maxYdiffPos.setY(y);
                    maxYdiffPos.setX(x / 2);
                }
                if (pixYdiff < pixYminDiff)
                {
                    pixYminDiff = pixYdiff;
                }
                pixYdiffSum += pixYdiff;
            }
        }
        int pixCnt = height * width;
        QString tmp = QString("Y max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixYmaxDiff).arg(pixYminDiff).arg(pixYdiffSum / (qreal)pixCnt).arg(maxYdiffPos.y()).arg(maxYdiffPos.x());

        for (int y = 0; y < height; y++)
        {
            for (int x = ux_start; x < width * 2; x += 4)
            {
                unsigned int l_U = bits <= 8 ? left[y * width * 2 + x] : ((quint16 *)left)[y * width * 2 + x];
                unsigned int r_U = bits <= 8 ? right[y * width * 2 + x] : ((quint16 *)right)[y * width * 2 + x];
                pixUdiff = l_U > r_U ? l_U - r_U : r_U - l_U;

                if (pixUdiff > pixUmaxDiff)
                {
                    pixUmaxDiff = pixUdiff;
                    maxUdiffPos.setX(x / 2);
                    maxUdiffPos.setY(y);
                }
                if (pixUdiff < pixUminDiff)
                {
                    pixUminDiff = pixUdiff;
                }
                pixUdiffSum += pixUdiff;
            }
        }
        tmp += QString("U max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixUmaxDiff).arg(pixUminDiff).arg(pixUdiffSum / (qreal)(pixCnt / 2)).arg(maxUdiffPos.y()).arg(maxUdiffPos.x());

        for (int y = 0; y < height; y++)
        {
            for (int x = vx_start; x < width * 2; x += 4)
            {
                unsigned int l_V = bits <= 8 ? left[y * width * 2 + x] : ((quint16 *)left)[y * width * 2 + x];
                unsigned int r_V = bits <= 8 ? right[y * width * 2 + x] : ((quint16 *)right)[y * width * 2 + x];
                pixVdiff = l_V > r_V ? l_V - r_V : r_V - l_V;

                if (pixVdiff > pixVmaxDiff)
                {
                    pixVmaxDiff = pixVdiff;
                    maxVdiffPos.setX(x / 2);
                    maxVdiffPos.setY(y);
                }
                if (pixVdiff < pixVminDiff)
                {
                    pixVminDiff = pixVdiff;
                }
                pixVdiffSum += pixVdiff;
            }
        }
        tmp += QString("V max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixVmaxDiff).arg(pixVminDiff).arg(pixVdiffSum / (qreal)(pixCnt / 2)).arg(maxVdiffPos.y()).arg(maxVdiffPos.x());

        return tmp;
    }
    else if(yuvTp == YuvType::YUV444_INTERLEAVE) // YUVYUV
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width * 3; x += 3)
            {
                unsigned int l_gray = bits <= 8 ? left[y * width * 3 + x] : ((quint16 *)left)[y * width * 3 + x];
                unsigned int r_gray = bits <= 8 ? right[y * width * 3 + x] : ((quint16 *)right)[y * width * 3 + x];
                pixYdiff = l_gray > r_gray ? l_gray - r_gray : r_gray - l_gray;

                if (pixYdiff > pixYmaxDiff)
                {
                    pixYmaxDiff = pixYdiff;
                    maxYdiffPos.setY(y);
                    maxYdiffPos.setX(x / 3);
                }
                if (pixYdiff < pixYminDiff)
                {
                    pixYminDiff = pixYdiff;
                }
                pixYdiffSum += pixYdiff;
            }
        }
        int pixCnt = height * width;
        QString tmp = QString("Y max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixYmaxDiff).arg(pixYminDiff).arg(pixYdiffSum / (qreal)pixCnt).arg(maxYdiffPos.y()).arg(maxYdiffPos.x());

        for (int y = 0; y < height; y++)
        {
            for (int x = 1; x < width * 3; x += 3)
            {
                unsigned int l_u = bits <= 8 ? left[y * width * 3 + x] : ((quint16 *)left)[y * width * 3 + x];
                unsigned int r_u = bits <= 8 ? right[y * width * 3 + x] : ((quint16 *)right)[y * width * 3 + x];
                pixUdiff = l_u > r_u ? l_u - r_u : r_u - l_u;

                if (pixUdiff > pixUmaxDiff)
                {
                    pixUmaxDiff = pixUdiff;
                    maxUdiffPos.setY(y);
                    maxUdiffPos.setX(x / 3);
                }
                if (pixUdiff < pixUminDiff)
                    pixUminDiff = pixUdiff;
                pixUdiffSum += pixUdiff;
            }
        }
        tmp += QString("U max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixUmaxDiff).arg(pixUminDiff).arg(pixUdiffSum / (qreal)pixCnt).arg(maxUdiffPos.y()).arg(maxUdiffPos.x());

        for (int y = 0; y < height; y++)
        {
            for (int x = 2; x < width * 3; x += 3)
            {
                unsigned int l_v = bits <= 8 ? left[y * width * 3 + x] : ((quint16 *)left)[y * width * 3 + x];
                unsigned int r_v = bits <= 8 ? right[y * width * 3 + x] : ((quint16 *)right)[y * width * 3 + x];
                pixVdiff = l_v > r_v ? l_v - r_v : r_v - l_v;

                if (pixVdiff > pixVmaxDiff)
                {
                    pixVmaxDiff = pixVdiff;
                    maxVdiffPos.setY(y);
                    maxVdiffPos.setX(x / 3);
                }
                if (pixVdiff < pixVminDiff)
                {
                    pixVminDiff = pixVdiff;
                }
                pixVdiffSum += pixVdiff;
            }
        }
        tmp += QString("V max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixUmaxDiff).arg(pixUminDiff).arg(pixUdiffSum / (qreal)pixCnt).arg(maxVdiffPos.y()).arg(maxVdiffPos.x());
        return tmp;
    }
    else if(yuvTp == YuvType::YUV444_PLANAR) // YYY..UUU..VVV
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                unsigned int l_gray = bits <= 8 ? left[y * width + x] : ((quint16 *)left)[y * width + x];
                unsigned int r_gray = bits <= 8 ? right[y * width + x] : ((quint16 *)right)[y * width + x];
                pixYdiff = l_gray > r_gray ? l_gray - r_gray : r_gray - l_gray;

                if (pixYdiff > pixYmaxDiff)
                {
                    pixYmaxDiff = pixYdiff;
                    maxYdiffPos.setY(y);
                    maxYdiffPos.setX(x);
                }
                if (pixYdiff < pixYminDiff)
                {
                    pixYminDiff = pixYdiff;
                }
                pixYdiffSum += pixYdiff;
            }
        }
        int pixCnt = height * width;
        QString tmp = QString("Y max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixYmaxDiff).arg(pixYminDiff).arg(pixYdiffSum / (qreal)pixCnt).arg(maxYdiffPos.y()).arg(maxYdiffPos.x());

        const unsigned char* left_u = left + pixCnt * (bits > 8 ? 2 : 1);
        const unsigned char* right_u = right + pixCnt * (bits > 8 ? 2 : 1);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                unsigned int l_U = bits <= 8 ? left_u[y * width + x] : ((quint16 *)left_u)[y * width + x];
                unsigned int r_U = bits <= 8 ? right_u[y * width + x] : ((quint16 *)right_u)[y * width + x];
                pixUdiff = l_U > r_U ? l_U - r_U : r_U - l_U;

                if (pixUdiff > pixUmaxDiff)
                {
                    pixUmaxDiff = pixUdiff;
                    maxUdiffPos.setY(y);
                    maxUdiffPos.setX(x);
                }
                if (pixUdiff < pixUminDiff)
                {
                    pixUminDiff = pixUdiff;
                }
                pixUdiffSum += pixUdiff;
            }
        }
        tmp += QString("U max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixUmaxDiff).arg(pixUminDiff).arg(pixUdiffSum / (qreal)pixCnt).arg(maxUdiffPos.y()).arg(maxUdiffPos.x());

        const unsigned char* left_v = left_u + pixCnt * (bits > 8 ? 2 : 1);
        const unsigned char* right_v = right_u + pixCnt * (bits > 8 ? 2 : 1);
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                unsigned int l_V = bits <= 8 ? left_v[y * width + x] : ((quint16 *)left_v)[y * width + x];
                unsigned int r_V = bits <= 8 ? right_v[y * width + x] : ((quint16 *)right_v)[y * width + x];
                pixVdiff = l_V > r_V ? l_V - r_V : r_V - l_V;

                if (pixVdiff > pixVmaxDiff)
                {
                    pixVmaxDiff = pixVdiff;
                    maxVdiffPos.setY(y);
                    maxVdiffPos.setX(x);
                }
                if (pixVdiff < pixVminDiff)
                {
                    pixVminDiff = pixVdiff;
                }
                pixVdiffSum += pixVdiff;
            }
        }
        tmp += QString("V max diff:%1 @ [%4, %5], min diff:%2, mean diff:%3\n").arg(pixVmaxDiff).arg(pixVminDiff).arg(pixVdiffSum / (qreal)pixCnt).arg(maxVdiffPos.y()).arg(maxVdiffPos.x());
        return tmp;
    }
    else
    {
        QString tmp("error yuv format\n");
        return tmp;
    }
}

void IIPviewer::showImageDiffReport()
{
    if(ui.imageLabel[0]->pixMap == nullptr || ui.imageLabel[1]->pixMap == nullptr || openedFile[0].isEmpty() || openedFile[1].isEmpty())
    {
        QMessageBox::critical(this, tr("error"), tr("no image exist on left or right side!"), QMessageBox::StandardButton::Close);
        return;
    }
    if(ui.imageLabel[0]->openedImgType != ui.imageLabel[1]->openedImgType)
    {
        QMessageBox::critical(this, tr("error"), tr("left image type not same as right side!"), QMessageBox::StandardButton::Close);
        return;
    }
    if(originSize[0] != originSize[1])
    {
        QMessageBox::critical(this, tr("error"), tr("left image size not equal to right side!"), QMessageBox::StandardButton::Close);
        return;
    }

    QString diffInfo = QString("%1 <------> %2\n").arg(openedFile[0]).arg(openedFile[1]);
    if(ui.imageLabel[0]->openedImgType == OpenedImageType::NORMAL_IMG)
    {
        diffInfo += compareNormalImage(ui.imageLabel[0]->pixMap, ui.imageLabel[1]->pixMap);
    }
    else if(ui.imageLabel[0]->openedImgType == OpenedImageType::PGM_IMG)
    {
        if(ui.imageLabel[0]->pgmDataBit != ui.imageLabel[1]->pgmDataBit)
        {
            QMessageBox::critical(this, tr("error"), tr("left image bit width not equal to right side!"), QMessageBox::StandardButton::Close);
            return;
        }
        diffInfo += comparePgmImage(ui.imageLabel[0]->pgmDataPtr, ui.imageLabel[1]->pgmDataPtr, ui.imageLabel[0]->pgmDataBit, originSize[0]);
    }
    else if(ui.imageLabel[0]->openedImgType == OpenedImageType::PNM_IMG)
    {
        if(ui.imageLabel[0]->pnmDataBit != ui.imageLabel[1]->pnmDataBit)
        {
            QMessageBox::critical(this, tr("error"), tr("left image bit width not equal to right side!"), QMessageBox::StandardButton::Close);
            return;
        }
        diffInfo += comparePnmImage(ui.imageLabel[0]->pnmDataPtr, ui.imageLabel[1]->pnmDataPtr, ui.imageLabel[0]->pnmDataBit, originSize[0]);
    }
    else if(ui.imageLabel[0]->openedImgType == OpenedImageType::RAW_IMG)
    {
        if(ui.imageLabel[0]->rawDataBit != ui.imageLabel[1]->rawDataBit)
        {
            QMessageBox::critical(this, tr("error"), tr("left image bit width not equal to right side!"), QMessageBox::StandardButton::Close);
            return;
        }
        diffInfo += compareRawImage(ui.imageLabel[0]->rawDataPtr, ui.imageLabel[1]->rawDataPtr, ui.imageLabel[0]->rawDataBit, originSize[0]);
    }
    else if(ui.imageLabel[0]->openedImgType == OpenedImageType::YUV_IMG)
    {
        if(ui.imageLabel[0]->yuvDataBit != ui.imageLabel[1]->yuvDataBit)
        {
            QMessageBox::critical(this, tr("error"), tr("left image bit width not equal to right side!"), QMessageBox::StandardButton::Close);
            return;
        }
        if(ui.imageLabel[0]->yuvType != ui.imageLabel[1]->yuvType)
        {
            QMessageBox::critical(this, tr("error"), tr("left image yuv type not equal to right side!"), QMessageBox::StandardButton::Close);
            return;
        }
        diffInfo += compareYuvImage(ui.imageLabel[0]->yuvDataPtr, ui.imageLabel[1]->yuvDataPtr, ui.imageLabel[0]->yuvDataBit, ui.imageLabel[0]->yuvType, originSize[0]);
    }

    ImgDiffReportDlg dlg(this);
    
    dlg.setReportInfo(diffInfo);
    dlg.exec();
}
