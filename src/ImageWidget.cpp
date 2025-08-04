#include "ImageWidget.h"
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QAction>
#include <stdexcept>

#define CLIP3(a, mi, ma) (a < mi ? mi : (a > ma ? ma : a))

ImageWidget::ImageWidget(QColor color, int penWidth, QScrollArea *parentScroll, QWidget *parent)
    : QWidget(parent),
      parentScroll(parentScroll),
      mouseAction(NONE_ACTION),
      penColor(color),
      penWidth(penWidth),
      ptCodInfo{{QPoint(0, 0), QPoint(0, 0)}, {QPoint(0, 0), QPoint(0, 0)}, 1.0},
      paintBegin(false), paintEnd(false),
      doDragImg(false), imgDragStartPos(0, 0), imgDragEndPos(0, 0),
      pixMap(nullptr),
      zoomIdx(2), zoomList{0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 12.0, 16.0, 24.0, 32.0, 48.0, 64.0, 96.0},
      zoomTextRect(), 
      pixValPaintRect(), 
      rawDataPtr(nullptr), rawDataBit(0),
      pnmDataPtr(nullptr), pnmDataBit(0), pgmDataPtr(nullptr), pgmDataBit(0),
      yuvDataPtr(nullptr), yuvDataBit(0), 
      rawBayerType(RawFileInfoDlg::BayerPatternType::BAYER_UNKNOW), rawByteOrderType(RawFileInfoDlg::RAW_LITTLE_ENDIAN), yuvType(YuvFileInfoDlg::YuvType::YUV_UNKNOW),
      openedImgType(UNKNOW_IMG),
      rightMouseContextMenu(this)
{
    QAction* exportPython = rightMouseContextMenu.addAction(tr("export roi data"));
    connect(exportPython, &QAction::triggered, this, &ImageWidget::exportRoiData);
}

ImageWidget::~ImageWidget()
{
    releaseBuffer();
}

static const RawFileInfoDlg::BayerPixelType type_RGGB[4] = {RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B};
static const RawFileInfoDlg::BayerPixelType type_GRBG[4] = {RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB};
static const RawFileInfoDlg::BayerPixelType type_GBRG[4] = {RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR};
static const RawFileInfoDlg::BayerPixelType type_BGGR[4] = {RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R};

static const RawFileInfoDlg::BayerPixelType type_RGG_IR[16] = {
    RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB,
    RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_IR, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y,
    RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR,
    RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y
};
static const RawFileInfoDlg::BayerPixelType type_BGG_IR[16] = {
    RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR,
    RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_IR, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y,
    RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB,
    RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y
};
static const RawFileInfoDlg::BayerPixelType type_GR_IR_G[16] = {
    RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B,
    RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR,
    RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R,
    RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB
};
static const RawFileInfoDlg::BayerPixelType type_GB_IR_G[16] = {
    RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R,
    RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB,
    RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B,
    RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR
};
static const RawFileInfoDlg::BayerPixelType type_G_IR_RG[16] = {
    RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y,
    RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB,
    RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y,
    RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR
};
static const RawFileInfoDlg::BayerPixelType type_G_IR_BG[16] = {
    RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y,
    RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR,
    RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y,
    RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GB
};
static const RawFileInfoDlg::BayerPixelType type_IR_GGR[16] = {
    RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR,
    RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B,
    RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB,
    RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R
};
static const RawFileInfoDlg::BayerPixelType type_IR_GGB[16] = {
    RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB,
    RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B, RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R,
    RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_Y, RawFileInfoDlg::PIX_GR,
    RawFileInfoDlg::PIX_GR, RawFileInfoDlg::PIX_R, RawFileInfoDlg::PIX_GB, RawFileInfoDlg::PIX_B
};

static const RawFileInfoDlg::BayerPixelType* type_rgbir[8] = {
    type_RGG_IR, type_BGG_IR, type_GR_IR_G, type_GB_IR_G, type_G_IR_RG, type_G_IR_BG, type_IR_GGR, type_IR_GGB
};

RawFileInfoDlg::BayerPixelType ImageWidget::getPixType(int y, int x, RawFileInfoDlg::BayerPatternType by)
{
    if (by >= RawFileInfoDlg::BayerPatternType::RGGB && by <= RawFileInfoDlg::BayerPatternType::BGGR)
    {
        uint32_t pos = ((y & 0x1) << 1) + (x & 0x1);

        const RawFileInfoDlg::BayerPixelType *type = type_RGGB;
        if (by == RawFileInfoDlg::BayerPatternType::RGGB)
        {
            type = type_RGGB;
        }
        else if (by == RawFileInfoDlg::BayerPatternType::GRBG)
        {
            type = type_GRBG;
        }
        else if (by == RawFileInfoDlg::BayerPatternType::GBRG)
        {
            type = type_GBRG;
        }
        else if (by == RawFileInfoDlg::BayerPatternType::BGGR)
        {
            type = type_BGGR;
        }

        return type[pos];
    }
    else if(by >= RawFileInfoDlg::BayerPatternType::RGGIR && by <= RawFileInfoDlg::BayerPatternType::IRGGB)
    {
        uint32_t pos = ((y & 0x11) << 2) + (x & 0x11);
        const RawFileInfoDlg::BayerPixelType *type = type_rgbir[by - RawFileInfoDlg::BayerPatternType::RGGIR];
        return type[pos];
    }
    else if(by == RawFileInfoDlg::BayerPatternType::MONO)
    {
        return RawFileInfoDlg::BayerPixelType::PIX_Y;
    }
    return RawFileInfoDlg::BayerPixelType::PIX_UNKNOW;
}

void ImageWidget::paintBitMapPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int start_x = viewTopLeftPix.x();
    int start_y = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    painter.setPen(QColor(255, 0, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 21);
            painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("%d", pixMap->pixelColor(start_x + w, start_y + h).red()));
        }
    }
    painter.setPen(QColor(0, 255, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21, 64, 21);
            painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("%d", pixMap->pixelColor(start_x + w, start_y + h).green()));
        }
    }
    painter.setPen(QColor(0, 0, 255));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21 + 21, 64, 21);
            painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("%d", pixMap->pixelColor(start_x + w, start_y + h).blue()));
        }
    }
}

void ImageWidget::paintRawPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 32, 64, 32);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int rawWidth = pixMap->width();
    int rawHeight = pixMap->height();

    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < rawHeight && xStart + w < rawWidth)
            {
                RawFileInfoDlg::BayerPixelType pixType = getPixType(yStart + h, xStart + w, rawBayerType);
                if (pixType == RawFileInfoDlg::BayerPixelType::PIX_R)
                {
                    painter.setPen(QColor(255, 0, 0));
                }
                else if (pixType == RawFileInfoDlg::BayerPixelType::PIX_GR || pixType == RawFileInfoDlg::BayerPixelType::PIX_GB)
                {
                    painter.setPen(QColor(0, 255, 0));
                }
                else if (pixType == RawFileInfoDlg::BayerPixelType::PIX_B)
                {
                    painter.setPen(QColor(0, 0, 255));
                }
                else if (pixType == RawFileInfoDlg::BayerPixelType::PIX_Y)
                {
                    painter.setPen(QColor(200, 200, 200));
                }
                else if (pixType == RawFileInfoDlg::BayerPixelType::PIX_IR)
                {
                    painter.setPen(QColor(128, 128, 128));
                }
                unsigned int gray = 0;
                if (rawDataBit <= 8)
                {
                    gray = ((unsigned char *)rawDataPtr)[(yStart + h) * rawWidth + xStart + w];
                }
                else if (rawDataBit > 8 && rawDataBit <= 16)
                {
                    gray = ((unsigned short *)rawDataPtr)[(yStart + h) * rawWidth + xStart + w];
                    if (rawByteOrderType == RawFileInfoDlg::RAW_BIG_ENDIAN)
                    {
                        gray = ((gray & 0x00ff) << 8) | ((gray & 0xff00) >> 8);
                    }
                }
                else if (rawDataBit > 16 && rawDataBit <= 32)
                {
                    gray = ((unsigned int *)rawDataPtr)[(yStart + h) * rawWidth + xStart + w];
                    if (rawByteOrderType == RawFileInfoDlg::RAW_BIG_ENDIAN)
                    {
                        gray = ((gray & 0x000000ff) << 24) | ((gray & 0x0000ff00) << 8) | ((gray & 0x00ff0000) >> 8) | ((gray & 0xff000000) >> 24);
                    }
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 32, 64, 32);
                
                QFont font = painter.font();
                font.setPixelSize(12); // 减小字体大小确保不超出
                painter.setFont(font);
                painter.drawText(pixValRect, Qt::AlignCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere, QString::asprintf("%d", gray));
            }
        }
    }
}

void ImageWidget::paintPnmPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int pnmWidth = pixMap->width();
    int pnmHeight = pixMap->height();

    bool isGray = (pixMap->format() == QImage::Format_Grayscale8);

    if (isGray)
    {
        painter.setPen(QColor(96, 96, 96));
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                if (yStart + h < pnmHeight && xStart + w < pnmWidth)
                {
                    unsigned int gray = 0;
                    if (pnmDataBit <= 8)
                    {
                        gray = ((unsigned char *)pnmDataPtr)[(yStart + h) * pnmWidth + (xStart + w)];
                    }
                    else if (pnmDataBit > 8 && pnmDataBit <= 16)
                    {
                        gray = ((unsigned short *)pnmDataPtr)[(yStart + h) * pnmWidth + (xStart + w)];
                    }

                    QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 32, 64, 32);

                    painter.drawText(pixValRect, Qt::AlignCenter | Qt::TextWordWrap, QString::asprintf("%d", gray));
                }
            }
        }
        return;
    }

    painter.setPen(QColor(255, 0, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < pnmHeight && xStart + w < pnmWidth)
            {
                unsigned int r = 0;
                if (pnmDataBit <= 8)
                {
                    r = ((unsigned char *)pnmDataPtr)[(yStart + h) * pnmWidth * 3 + (xStart + w) * 3];
                }
                else if (pnmDataBit > 8 && pnmDataBit <= 16)
                {
                    r = ((unsigned short *)pnmDataPtr)[(yStart + h) * pnmWidth * 3 + (xStart + w) * 3];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter | Qt::TextWordWrap, QString::asprintf("%d", r));
            }
        }
    }

    painter.setPen(QColor(0, 255, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < pnmHeight && xStart + w < pnmWidth)
            {
                unsigned int g = 0;
                if (pnmDataBit <= 8)
                {
                    g = ((unsigned char *)pnmDataPtr)[(yStart + h) * pnmWidth * 3 + (xStart + w) * 3 + 1];
                }
                else if (pnmDataBit > 8 && pnmDataBit <= 16)
                {
                    g = ((unsigned short *)pnmDataPtr)[(yStart + h) * pnmWidth * 3 + (xStart + w) * 3 + 1];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter | Qt::TextWordWrap, QString::asprintf("%d", g));
            }
        }
    }
    painter.setPen(QColor(0, 0, 255));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < pnmHeight && xStart + w < pnmWidth)
            {
                unsigned int b = 0;
                if (pnmDataBit <= 8)
                {
                    b = ((unsigned char *)pnmDataPtr)[(yStart + h) * pnmWidth * 3 + (xStart + w) * 3 + 2];
                }
                else if (pnmDataBit > 8 && pnmDataBit <= 16)
                {
                    b = ((unsigned short *)pnmDataPtr)[(yStart + h) * pnmWidth * 3 + (xStart + w) * 3 + 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter | Qt::TextWordWrap, QString::asprintf("%d", b));
            }
        }
    }
}

void ImageWidget::paintPgmPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 32, 64, 32);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int pgmWidth = pixMap->width();
    int pgmHeight = pixMap->height();

    painter.setPen(QColor(96, 96, 96));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < pgmHeight && xStart + w < pgmWidth)
            {
                unsigned int gray = 0;
                if (pgmDataBit <= 8)
                {
                    gray = ((unsigned char *)pgmDataPtr)[(yStart + h) * pgmWidth + (xStart + w)];
                }
                else if (pgmDataBit > 8 && pgmDataBit <= 16)
                {
                    gray = ((unsigned short *)pgmDataPtr)[(yStart + h) * pgmWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 32);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("%d", gray));
            }
        }
    }
}

void ImageWidget::paintYuv444InterleavePixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    int uv_disp_mode = appSettings->uv_value_disp_mode;

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 3 + (xStart + w) * 3];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 3 + (xStart + w) * 3];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter | Qt::TextWordWrap, QString::asprintf("Y:%d", y));
            }
        }
    }

    painter.setPen(QColor(0, 50, 255));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int u = 0;
                if (yuvDataBit <= 8)
                {
                    u = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 3 + (xStart + w) * 3 + 1];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    u = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 3 + (xStart + w) * 3 + 1];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter | Qt::TextWordWrap, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
            }
        }
    }
    painter.setPen(QColor(255, 50, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int v = 0;
                if (yuvDataBit <= 8)
                {
                    v = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 3 + (xStart + w) * 3 + 2];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    v = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 3 + (xStart + w) * 3 + 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter | Qt::TextWordWrap, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
            }
        }
    }
}

void ImageWidget::paintYuv444PlanarPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    int uv_disp_mode = appSettings->uv_value_disp_mode;

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("Y:%d", y));
            }
        }
    }

    painter.setPen(QColor(0, 50, 255));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int u = 0;
                if (yuvDataBit <= 8)
                {
                    u = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + (yStart + h) * yuvWidth + (xStart + w)];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    u = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + (yStart + h) * yuvWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
            }
        }
    }
    painter.setPen(QColor(255, 50, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int v = 0;
                if (yuvDataBit <= 8)
                {
                    v = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight * 2 + (yStart + h) * yuvWidth + (xStart + w)];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    v = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight * 2 + (yStart + h) * yuvWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
            }
        }
    }
}

void ImageWidget::paintYuv422UYVYPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    int uv_disp_mode = appSettings->uv_value_disp_mode;

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2 + 1];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2 + 1];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 32);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("Y:%d", y));
            }
        }
    }

    if ((xStart & 0x1) == 0x0)
    {
        painter.setPen(QColor(0, 50, 255));
    }
    else
    {
        painter.setPen(QColor(255, 50, 0));
    }
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w += 2)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int u = 0;
                if (yuvDataBit <= 8)
                {
                    u = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    u = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 32, 64, 32);

                if ((xStart & 0x1) == 0x0)
                {
                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
                }
                else
                {
                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)): (int)u));
                }
            }
        }
    }
    if ((xStart & 0x1) == 0x0)
    {
        painter.setPen(QColor(255, 50, 0));
    }
    else
    {
        painter.setPen(QColor(0, 50, 255));
    }
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 1; w < viewPixWidth; w += 2)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int v = 0;
                if (yuvDataBit <= 8)
                {
                    v = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    v = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 32, 64, 32);

                if ((xStart & 0x1) == 0x0)
                {
                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
                }
                else
                {
                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
                }
            }
        }
    }
}

void ImageWidget::paintYuv422YUYVPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    int uv_disp_mode = appSettings->uv_value_disp_mode;

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 32);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("Y:%d", y));
            }
        }
    }

    if ((xStart & 0x1) == 0x0)
    {
        painter.setPen(QColor(0, 50, 255));
    }
    else
    {
        painter.setPen(QColor(255, 50, 0));
    }
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w += 2)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int u = 0;
                if (yuvDataBit <= 8)
                {
                    u = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2 + 1];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    u = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2 + 1];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 32, 64, 32);
                if ((xStart & 0x1) == 0x0)
                {
                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
                }
                else
                {
                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
                }
            }
        }
    }
    if ((xStart & 0x1) == 0x0)
    {
        painter.setPen(QColor(255, 50, 0));
    }
    else
    {
        painter.setPen(QColor(0, 50, 255));
    }
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 1; w < viewPixWidth; w += 2)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int v = 0;
                if (yuvDataBit <= 8)
                {
                    v = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2 + 1];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    v = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth * 2 + (xStart + w) * 2 + 1];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 32, 64, 32);

                if ((xStart & 0x1) == 0x0)
                {
                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
                }
                else
                {
                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
                }
            }
        }
    }
}

// YYYYYYYY
// UVUVUVUV
//
void ImageWidget::paintYuv420NV12PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    int uv_disp_mode = appSettings->uv_value_disp_mode;

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("Y:%d", y));
            }
        }
    }

    painter.setPen(QColor(0, 50, 255));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (((yStart + h) & 0x1) == 0x0 && ((xStart + w) & 0x1) == 0x0)
            {
                if (yStart + h < yuvHeight && xStart + w < yuvWidth)
                {
                    unsigned int u = 0;
                    if (yuvDataBit <= 8)
                    {
                        u = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth + (xStart + w)];
                    }
                    else if (yuvDataBit > 8 && yuvDataBit <= 16)
                    {
                        u = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth + (xStart + w)];
                    }

                    QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21, 64, 21);

                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
                }
            }
        }
    }
    painter.setPen(QColor(255, 50, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (((yStart + h) & 0x1) == 0x0 && ((xStart + w) & 0x1) == 0x0)
            {
                if (yStart + h < yuvHeight && xStart + w < yuvWidth)
                {
                    unsigned int v = 0;
                    if (yuvDataBit <= 8)
                    {
                        v = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth + (xStart + w) + 1];
                    }
                    else if (yuvDataBit > 8 && yuvDataBit <= 16)
                    {
                        v = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth + (xStart + w) + 1];
                    }

                    QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21 + 21, 64, 21);

                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
                }
            }
        }
    }
}

void ImageWidget::paintYuv420NV21PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    int uv_disp_mode = appSettings->uv_value_disp_mode;

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 21);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("Y:%d", y));
            }
        }
    }

    painter.setPen(QColor(0, 50, 255));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (((yStart + h) & 0x1) == 0x0 && ((xStart + w) & 0x1) == 0x0)
            {
                if (yStart + h < yuvHeight && xStart + w < yuvWidth)
                {
                    unsigned int u = 0;
                    if (yuvDataBit <= 8)
                    {
                        u = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth + (xStart + w) + 1];
                    }
                    else if (yuvDataBit > 8 && yuvDataBit <= 16)
                    {
                        u = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth + (xStart + w) + 1];
                    }

                    QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21, 64, 21);

                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
                }
            }
        }
    }
    painter.setPen(QColor(255, 50, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (((yStart + h) & 0x1) == 0x0 && ((xStart + w) & 0x1) == 0x0)
            {
                if (yStart + h < yuvHeight && xStart + w < yuvWidth)
                {
                    unsigned int v = 0;
                    if (yuvDataBit <= 8)
                    {
                        v = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth + (xStart + w)];
                    }
                    else if (yuvDataBit > 8 && yuvDataBit <= 16)
                    {
                        v = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth + (xStart + w)];
                    }

                    QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21 + 21, 64, 21);

                    painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
                }
            }
        }
    }
}

void ImageWidget::paintYuv420PYU12PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    int uv_disp_mode = appSettings->uv_value_disp_mode;

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 21);

                painter.drawText(pixValRect, Qt::AlignmentFlag::AlignCenter | Qt::TextWordWrap, QString::asprintf("Y:%d", y));
            }
        }
    }

    painter.setPen(QColor(0, 50, 255));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int u = 0;
                if (yuvDataBit <= 8)
                {
                    u = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth / 2 + (xStart + w) / 2];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    u = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth / 2 + (xStart + w) / 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignmentFlag::AlignCenter | Qt::TextWordWrap, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
            }
        }
    }
    painter.setPen(QColor(255, 50, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int v = 0;
                if (yuvDataBit <= 8)
                {
                    v = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + yuvWidth * yuvHeight / 4 + ((yStart + h) / 2) * yuvWidth / 2 + (xStart + w) / 2];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    v = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + yuvWidth * yuvHeight / 4 + ((yStart + h) / 2) * yuvWidth / 2 + (xStart + w) / 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignmentFlag::AlignCenter | Qt::TextWordWrap, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
            }
        }
    }
}

void ImageWidget::paintYuv420PYV12PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 64);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    int uv_disp_mode = appSettings->uv_value_disp_mode;

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16, 64, 21);

                painter.drawText(pixValRect, Qt::AlignmentFlag::AlignCenter, QString::asprintf("Y:%d", y));
            }
        }
    }

    painter.setPen(QColor(0, 50, 255));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int v = 0;
                if (yuvDataBit <= 8)
                {
                    v = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth / 2 + (xStart + w) / 2];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    v = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + ((yStart + h) / 2) * yuvWidth / 2 + (xStart + w) / 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignmentFlag::AlignCenter, QString::asprintf("U:%d", uv_disp_mode == 0 ? (int)v - (1 << (yuvDataBit - 1)) : (int)v));
            }
        }
    }
    painter.setPen(QColor(255, 50, 0));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int u = 0;
                if (yuvDataBit <= 8)
                {
                    u = ((unsigned char *)yuvDataPtr)[yuvWidth * yuvHeight + yuvWidth * yuvHeight / 4 + ((yStart + h) / 2) * yuvWidth / 2 + (xStart + w) / 2];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    u = ((unsigned short *)yuvDataPtr)[yuvWidth * yuvHeight + yuvWidth * yuvHeight / 4 + ((yStart + h) / 2) * yuvWidth / 2 + (xStart + w) / 2];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 16 + 21 + 21, 64, 21);

                painter.drawText(pixValRect, Qt::AlignmentFlag::AlignCenter, QString::asprintf("V:%d", uv_disp_mode == 0 ? (int)u - (1 << (yuvDataBit - 1)) : (int)u));
            }
        }
    }
}

void ImageWidget::paintYuv400PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft)
{
    int xStart = viewTopLeftPix.x();
    int yStart = viewTopLeftPix.y();

    if(appSettings->pix_val_bg_index != IIPOptionDialog::PaintPixValBgColor::NONE)
    {
        switch(appSettings->pix_val_bg_index)
        {
            case IIPOptionDialog::PaintPixValBgColor::CUSTOM:
                painter.setPen(appSettings->pix_val_cus_bg_color);
                painter.setBrush(appSettings->pix_val_cus_bg_color);
                break;
            case IIPOptionDialog::PaintPixValBgColor::WHITE:
                painter.setPen(Qt::white);
                painter.setBrush(Qt::white);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GRAY:
                painter.setPen(Qt::gray);
                painter.setBrush(Qt::gray);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLACK:
                painter.setPen(Qt::black);
                painter.setBrush(Qt::black);
                break;
            case IIPOptionDialog::PaintPixValBgColor::RED:
                painter.setPen(Qt::red);
                painter.setBrush(Qt::red);
                break;
            case IIPOptionDialog::PaintPixValBgColor::YELLOW:
                painter.setPen(Qt::yellow);
                painter.setBrush(Qt::yellow);
                break;
            case IIPOptionDialog::PaintPixValBgColor::GREEN:
                painter.setPen(Qt::green);
                painter.setBrush(Qt::green);
                break;
            case IIPOptionDialog::PaintPixValBgColor::CYAN:
                painter.setPen(Qt::cyan);
                painter.setBrush(Qt::cyan);
                break;
            case IIPOptionDialog::PaintPixValBgColor::BLUE:
                painter.setPen(Qt::blue);
                painter.setBrush(Qt::blue);
                break;
            case IIPOptionDialog::PaintPixValBgColor::MAGENTA:
                painter.setPen(Qt::magenta);
                painter.setBrush(Qt::magenta);
                break;
            default:
                break;
        }
        for (int h = 0; h < viewPixHeight; h++)
        {
            for (int w = 0; w < viewPixWidth; w++)
            {
                QRectF pixValBgRect(paintPixValTopLeft.x() + w * 96 + 16, paintPixValTopLeft.y() + h * 96 + 32, 64, 32);
                painter.drawRect(pixValBgRect);
            }
        }
    }

    int yuvWidth = pixMap->width();
    int yuvHeight = pixMap->height();

    painter.setPen(QColor(200, 200, 200));
    for (int h = 0; h < viewPixHeight; h++)
    {
        for (int w = 0; w < viewPixWidth; w++)
        {
            if (yStart + h < yuvHeight && xStart + w < yuvWidth)
            {
                unsigned int y = 0;
                if (yuvDataBit <= 8)
                {
                    y = ((unsigned char *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }
                else if (yuvDataBit > 8 && yuvDataBit <= 16)
                {
                    y = ((unsigned short *)yuvDataPtr)[(yStart + h) * yuvWidth + (xStart + w)];
                }

                QRectF pixValRect(paintPixValTopLeft.x() + w * 96, paintPixValTopLeft.y() + h * 96, 96, 32);

                painter.drawText(pixValRect, Qt::AlignCenter, QString::asprintf("Y:%d", y));
            }
        }
    }
}

void ImageWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    if (pixMap == nullptr)
    {
        QSize curSize = size();
        QFont font = painter.font();
        font.setPixelSize(28);
        painter.setFont(font);

        qreal radius = qMin(curSize.height(), curSize.width()) / 16;
        QPen cusPen(Qt::CustomDashLine);
        cusPen.setWidth(6);
        cusPen.setColor(QColor(180, 180, 180));
        QVector<qreal> dashes{4, 4};
        cusPen.setDashPattern(dashes);
        painter.setPen(cusPen);
        painter.setRenderHint(QPainter::Antialiasing);

        QFontMetrics fm(painter.font());
        int textHeight = fm.height();
        int spacing = textHeight / 2;
        int totalHeight = textHeight * 3 + spacing * 2;
        int startY = (curSize.height() - totalHeight) / 2;

        QRect appearRect0 = painter.boundingRect(QRect(0, startY, curSize.width(), textHeight), Qt::AlignmentFlag::AlignHCenter, tr("drag image file here"));
        painter.drawText(appearRect0, tr("drag image file here"));
        
        QRect appearRect1 = painter.boundingRect(QRect(0, startY + textHeight + spacing, curSize.width(), textHeight), Qt::AlignmentFlag::AlignHCenter, tr("or"));
        painter.drawText(appearRect1, tr("or"));
        
        QRect appearRect2 = painter.boundingRect(QRect(0, startY + (textHeight + spacing) * 2, curSize.width(), textHeight), Qt::AlignmentFlag::AlignHCenter, tr("double click here"));
        painter.drawText(appearRect2, tr("double click here"));

        painter.drawRoundedRect(QRect(10, 10, curSize.width() - 14, curSize.height() - 14), radius, radius);
        return;
    }
    QRect imgRect(pixMap->rect());
    float scaleRatio = zoomList[zoomIdx];
    QRect widgetRect(0, 0, imgRect.width() * scaleRatio, imgRect.height() * scaleRatio);
    painter.drawImage(widgetRect, *pixMap, imgRect);

    // 鼠标已有绘制或者正在绘制过程中
    if (paintBegin || paintEnd)
    {
        QPen pen(penColor);
        pen.setWidth(penWidth);
        painter.setPen(pen);
        painter.drawRect(ptCodInfo.paintCoordinates[0].x(), ptCodInfo.paintCoordinates[0].y(),
        ptCodInfo.paintCoordinates[1].x() - ptCodInfo.paintCoordinates[0].x(), ptCodInfo.paintCoordinates[1].y() - ptCodInfo.paintCoordinates[0].y());
        // qDebug() << ptCodInfo.paintCoordinates[0].x() << ", " << ptCodInfo.paintCoordinates[0].y() << "; " << ptCodInfo.paintCoordinates[1].x() - ptCodInfo.paintCoordinates[0].x() << ", " << ptCodInfo.paintCoordinates[1].y() - ptCodInfo.paintCoordinates[0].y();
    }
    if (scaleRatio >= 96.0)
    {
        int sclRatio = int(scaleRatio);
        int layoutLeftMargin = parentWidget()->layout()->contentsMargins().left();
        int layoutTopMargin = parentWidget()->layout()->contentsMargins().top();
        int viewLeftPos = layoutLeftMargin + parentWidget()->geometry().left();
        viewLeftPos = viewLeftPos > 0 ? 0 : viewLeftPos;
        int viewTopPos = layoutTopMargin + parentWidget()->geometry().top();
        viewTopPos = viewTopPos > 0 ? 0 : viewTopPos;
        QPoint viewTopLeft(-viewLeftPos, -viewTopPos);

        QPoint viewTopLeftPix = viewTopLeft / sclRatio;
        int viewPixWidth = int((parentScroll->width() - parentScroll->verticalScrollBar()->width()) / sclRatio) + 1;
        int viewPixHeight = int((parentScroll->height() - parentScroll->horizontalScrollBar()->height()) / sclRatio) + 1;
        //        qDebug() << parentWidget()->geometry().topLeft() << viewTopLeftPix;
        //        qDebug() << viewPixWidth << viewPixHeight;

        QPoint paintPixValTopLeft = viewTopLeftPix * sclRatio;
        pixValPaintRect = QRectF(paintPixValTopLeft.x(), paintPixValTopLeft.y(), 96 * viewPixWidth, 96 * viewPixHeight);
        QFont font = painter.font();
        font.setPixelSize(18);
        painter.setFont(font);

        if (openedImgType == NORMAL_IMG)
        {
            paintBitMapPixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
        }
        else if (openedImgType == RAW_IMG)
        {
            paintRawPixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
        }
        else if (openedImgType == PNM_IMG)
        {
            paintPnmPixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
        }
        else if (openedImgType == PGM_IMG)
        {
            paintPgmPixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
        }
        else if (openedImgType == YUV_IMG)
        {
            if (yuvType == YuvFileInfoDlg::YuvType::YUV444_INTERLEAVE)
            {
                paintYuv444InterleavePixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
            else if (yuvType == YuvFileInfoDlg::YuvType::YUV444_PLANAR)
            {
                paintYuv444PlanarPixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
            else if (yuvType == YuvFileInfoDlg::YuvType::YUV422_UYVY)
            {
                paintYuv422UYVYPixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
            else if (yuvType == YuvFileInfoDlg::YuvType::YUV422_YUYV)
            {
                paintYuv422YUYVPixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
            else if (yuvType == YuvFileInfoDlg::YuvType::YUV420_NV12)
            {
                paintYuv420NV12PixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
            else if (yuvType == YuvFileInfoDlg::YuvType::YUV420_NV21)
            {
                paintYuv420NV21PixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
            else if (yuvType == YuvFileInfoDlg::YuvType::YUV420P_YU12)
            {
                paintYuv420PYU12PixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
            else if (yuvType == YuvFileInfoDlg::YuvType::YUV420P_YV12)
            {
                paintYuv420PYV12PixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
            else if (yuvType == YuvFileInfoDlg::YuvType::YUV400)
            {
                paintYuv400PixVal(viewTopLeftPix, painter, viewPixWidth, viewPixHeight, paintPixValTopLeft);
            }
        }
    }
}

static bool pointInRectangle(const QPoint& in_pt, const QPoint& p0, const QPoint& p1)
{
    if(in_pt.x() >= qMin(p0.x(), p1.x()) && in_pt.x() < qMax(p0.x(), p1.x()))
    {
        if(in_pt.y() >= qMin(p0.y(), p1.y()) && in_pt.y() < qMax(p0.y(), p1.y()))
        {
            return true;
        }
    }
    return false;
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (pixMap == nullptr)
    {
        return;
    }
    if (event->button() == Qt::LeftButton && !paintBegin && mouseAction == MouseActionType::PAINT_ROI_ACTION)
    {
        paintBegin = true;
        paintEnd = false;
        ptCodInfo.paintCoordinates[0] = event->pos();
        float scale_ratio = zoomList[zoomIdx];
        if (scale_ratio >= 1.0)
        {
            if (ptCodInfo.paintCoordinates[0].x() % int(scale_ratio) > 0)
            {
                int newX = (ptCodInfo.paintCoordinates[0].x() / int(scale_ratio)) * int(scale_ratio);
                ptCodInfo.paintCoordinates[0].setX(newX);
            }
            if (ptCodInfo.paintCoordinates[0].y() % int(scale_ratio) > 0)
            {
                int newY = (ptCodInfo.paintCoordinates[0].y() / int(scale_ratio)) * int(scale_ratio);
                ptCodInfo.paintCoordinates[0].setY(newY);
            }
        }
        ptCodInfo.originPaintCoordinates[0].setX(ptCodInfo.paintCoordinates[0].x());
        ptCodInfo.originPaintCoordinates[0].setY(ptCodInfo.paintCoordinates[0].y());
        ptCodInfo.originScaleRatio = scale_ratio;
        emit inform_real_Pos(ptCodInfo.paintCoordinates[0] / scale_ratio, ptCodInfo.paintCoordinates[1] / scale_ratio);
    }
    else if (event->button() == Qt::LeftButton && mouseAction == MouseActionType::DRAG_IMG_ACTION)
    {
        imgDragStartPos = event->pos();
        doDragImg = true;
        setCursor(Qt::ClosedHandCursor);
    }
    else if(event->button() == Qt::RightButton && paintEnd)
    {
        // 检查roi坐标是否构成一个rectangel
        if(ptCodInfo.paintCoordinates[0] != ptCodInfo.paintCoordinates[1])
        {
            // 如果鼠标点在roi范围内，弹出右键菜单
            QPoint cur_pos = event->pos();
            if(pointInRectangle(cur_pos, ptCodInfo.paintCoordinates[0], ptCodInfo.paintCoordinates[1]))
            {                
                rightMouseContextMenu.exec(event->globalPos());
            }
        }
    }
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (pixMap == nullptr)
        return;
    if (event->button() == Qt::LeftButton && paintBegin && mouseAction == MouseActionType::PAINT_ROI_ACTION)
    {
        paintBegin = false;
        paintEnd = true;
        ptCodInfo.paintCoordinates[1] = event->pos();
        float scale_ratio = zoomList[zoomIdx];
        if (scale_ratio >= 1.0)
        {
            if (ptCodInfo.paintCoordinates[1].x() % int(scale_ratio) > 0)
            {
                int newX = ((ptCodInfo.paintCoordinates[1].x() / int(scale_ratio)) + 1) * int(scale_ratio);
                ptCodInfo.paintCoordinates[1].setX(newX);
            }
            if (ptCodInfo.paintCoordinates[1].y() % int(scale_ratio) > 0)
            {
                int newY = ((ptCodInfo.paintCoordinates[1].y() / int(scale_ratio)) + 1) * int(scale_ratio);
                ptCodInfo.paintCoordinates[1].setY(newY);
            }
        }
        ptCodInfo.originPaintCoordinates[1].setX(ptCodInfo.paintCoordinates[1].x());
        ptCodInfo.originPaintCoordinates[1].setY(ptCodInfo.paintCoordinates[1].y());
        ptCodInfo.originScaleRatio = scale_ratio;
        emit inform_real_Pos(ptCodInfo.paintCoordinates[0] / scale_ratio, ptCodInfo.paintCoordinates[1] / scale_ratio);
    }
    else if (event->button() == Qt::LeftButton && mouseAction == MouseActionType::DRAG_IMG_ACTION)
    {
        doDragImg = false;
        setCursor(Qt::ArrowCursor);
    }
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (pixMap == nullptr)
    {
        return;
    }
    if (paintBegin)
    {
        ptCodInfo.paintCoordinates[1] = event->pos();
        float scale_ratio = zoomList[zoomIdx];
        if (scale_ratio >= 1.0)
        {
            if (ptCodInfo.paintCoordinates[1].x() % int(scale_ratio) > 0)
            {
                int newX = ((ptCodInfo.paintCoordinates[1].x() / int(scale_ratio)) + 1) * int(scale_ratio);
                ptCodInfo.paintCoordinates[1].setX(newX);
            }
            if (ptCodInfo.paintCoordinates[1].y() % int(scale_ratio) > 0)
            {
                int newY = ((ptCodInfo.paintCoordinates[1].y() / int(scale_ratio)) + 1) * int(scale_ratio);
                ptCodInfo.paintCoordinates[1].setY(newY);
            }
        }
        ptCodInfo.originPaintCoordinates[1].setX(ptCodInfo.paintCoordinates[1].x());
        ptCodInfo.originPaintCoordinates[1].setY(ptCodInfo.paintCoordinates[1].y());
        ptCodInfo.originScaleRatio = scale_ratio;
        emit inform_real_Pos(ptCodInfo.paintCoordinates[0] / scale_ratio, ptCodInfo.paintCoordinates[1] / scale_ratio);
        repaint();
    }
    if (doDragImg && pixMap != nullptr)
    {
        float scaleRatio = zoomList[zoomIdx];
        if (scaleRatio >= 96.0)
        {
            update(pixValPaintRect.toRect());
        }
        else
        {
            update(zoomTextRect.toRect());
        }
        imgDragEndPos = event->pos();
        emit inform_drag_img(imgDragStartPos, imgDragEndPos);
    }
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (pixMap != nullptr)
    {
        return;
    }
    emit inform_open_file_selector();
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
    if (pixMap == nullptr)
        return;
    emit inform_change_master();
    int delta_y = event->angleDelta().y() / 120;
    if (delta_y > 0 && zoomIdx < ZOOM_LIST_LENGTH - 1)
    {
        processWheelZoom(event, 1);
    }
    else if (delta_y < 0 && zoomIdx > 0)
    {
        processWheelZoom(event, -1);
    }
    event->accept();
}

void ImageWidget::setPixmap(QString &img) // jpg, jpeg, bmp, png, pnm, pgm, tiff
{
    if (img.endsWith(".pgm", Qt::CaseInsensitive))
    {
        QFile input_f(img);
        input_f.open(QIODevice::ReadOnly);
        QByteArray p5 = input_f.readLine();
        QByteArray w_h = input_f.readLine();
        QByteArray maxVal = input_f.readLine();
        if (p5 != QString("P5\n"))
        {
            QMessageBox::critical(this, "error", "no P5 flag detected in portable gray image", QMessageBox::StandardButton::Ok);
            input_f.close();
            return;
        }
        int maxPixVal = maxVal.toInt();
        auto width_height = w_h.split(' ');
        width_height.removeAll(QByteArray(nullptr));
        if (width_height.size() != 2)
        {
            input_f.close();
            return;
        }
        int pixSize = 2;
        if (maxPixVal <= 255)
        {
            pixSize = 1;
        }
        else if (maxPixVal > 255 && maxPixVal <= 65535)
        {
            pixSize = 2;
        }
        int bitDepth = 8;
        if (maxPixVal <= 255)
        {
            bitDepth = 8;
        }
        else if (maxPixVal > 255 && maxPixVal <= 1023)
        {
            bitDepth = 10;
        }
        else if (maxPixVal > 1023 && maxPixVal <= 4095)
        {
            bitDepth = 12;
        }
        else if (maxPixVal > 4095 && maxPixVal <= 16383)
        {
            bitDepth = 14;
        }
        else if (maxPixVal > 16383 && maxPixVal <= 65535)
        {
            bitDepth = 16;
        }

        int width = width_height[0].toInt();
        int height = width_height[1].toInt();
        releaseBuffer();

        unsigned char *buffer = new unsigned char[pixSize * width * height];
        pixMap = new QImage(width, height, QImage::Format_Grayscale8);
        input_f.read((char *)buffer, qint64(pixSize) * width * height);
        input_f.close();

        unsigned char *bufferShow = pixMap->bits();
        int bytesperline = pixMap->bytesPerLine();

        if (pixSize == 1)
        {
            for (qint64 i = 0; i < height; i++)
            {
                for (qint64 j = 0; j < width; j++)
                {
                    bufferShow[i * bytesperline + j] = buffer[i * width + j];
                }
            }
        }
        else if (pixSize == 2)
        {
            unsigned short *buffer_us = (unsigned short *)buffer;
            for (qint64 i = 0; i < height; i++)
            {
                for (qint64 j = 0; j < width; j++)
                {
                    buffer_us[i * width + j] = ((buffer_us[i * width + j] & 0xff00) >> 8) | ((buffer_us[i * width + j] & 0x00ff) << 8);
                    bufferShow[i * bytesperline + j] = buffer_us[i * width + j] >> (bitDepth - 8);
                }
            }
        }

        rawDataPtr = nullptr;
        rawDataBit = 0;
        rawBayerType = RawFileInfoDlg::BayerPatternType::BAYER_UNKNOW;
        rawByteOrderType = RawFileInfoDlg::ByteOrderType::RAW_LITTLE_ENDIAN;
        yuvDataBit = 0;
        yuvDataPtr = nullptr;
        yuvType = YuvFileInfoDlg::YuvType::YUV_UNKNOW;
        pnmDataPtr = nullptr;
        pnmDataBit = 0;
        openedImgType = PGM_IMG;
        pgmDataPtr = buffer;
        pgmDataBit = bitDepth;

        resize(pixMap->size() * zoomList[zoomIdx]);
        repaint();
    }
    else if (img.endsWith(".pnm", Qt::CaseInsensitive))
    {
        bool isGray = false;
        QFile input_f(img);
        input_f.open(QIODevice::ReadOnly);
        QByteArray p6 = input_f.readLine();
        QByteArray w_h = input_f.readLine();
        QByteArray maxVal = input_f.readLine();
        if (p6 != QString("P6\n") && p6 != QString("P5\n"))
        {
            QMessageBox::critical(this, "error", "only color & gray pnm format supported", QMessageBox::StandardButton::Ok);
            input_f.close();
            return;
        }
        if (p6 == QString("P5\n"))
        {
            isGray = true;
        }
        int maxPixVal = maxVal.toInt();
        auto width_height = w_h.split(' ');
        width_height.removeAll(QByteArray(nullptr));
        if (width_height.size() != 2)
        {
            input_f.close();
            return;
        }

        int pixSize = 2;
        if (maxPixVal <= 255)
        {
            pixSize = 1;
        }
        else if (maxPixVal > 255 && maxPixVal <= 65535)
        {
            pixSize = 2;
        }

        int bitDepth = 8;
        if (maxPixVal <= 255)
        {
            bitDepth = 8;
        }
        else if (maxPixVal > 255 && maxPixVal <= 1023)
        {
            bitDepth = 10;
        }
        else if (maxPixVal > 1023 && maxPixVal <= 4095)
        {
            bitDepth = 12;
        }
        else if (maxPixVal > 4095 && maxPixVal <= 16383)
        {
            bitDepth = 14;
        }
        else if (maxPixVal > 16383 && maxPixVal <= 65535)
        {
            bitDepth = 16;
        }

        int width = width_height[0].toInt();
        int height = width_height[1].toInt();

        releaseBuffer();

        unsigned char *buffer = nullptr;
        if (isGray)
        {
            buffer = new unsigned char[pixSize * width * height];
            pixMap = new QImage(width, height, QImage::Format_Grayscale8);
            input_f.read((char *)buffer, qint64(pixSize) * width * height);
            input_f.close();

            int bytesperline = pixMap->bytesPerLine();
            unsigned char *bufferShow = pixMap->bits();

            if (pixSize == 1)
            {
                for (qint64 i = 0; i < height; i++)
                {
                    for (qint64 j = 0; j < width; j++)
                    {
                        bufferShow[i * bytesperline + j] = buffer[i * width + j];
                    }
                }
            }
            else if (pixSize == 2)
            {
                unsigned short *buffer_us = (unsigned short *)buffer;
                for (qint64 i = 0; i < height; i++)
                {
                    for (qint64 j = 0; j < width; j++)
                    {
                        buffer_us[i * width + j] = ((buffer_us[i * width + j] & 0xff00) >> 8) | ((buffer_us[i * width + j] & 0x00ff) << 8);
                        bufferShow[i * bytesperline + j] = buffer_us[i * width + j] >> (bitDepth - 8);
                    }
                }
            }
        }
        else
        {
            buffer = new unsigned char[pixSize * width * height * 3];
            pixMap = new QImage(width, height, QImage::Format_RGB888);
            input_f.read((char *)buffer, qint64(pixSize) * width * height * 3);
            input_f.close();

            int bytesperline = pixMap->bytesPerLine();
            unsigned char *bufferShow = pixMap->bits();

            if (pixSize == 1)
            {
                for (qint64 i = 0; i < height; i++)
                {
                    for (qint64 j = 0; j < width * 3; j++)
                    {
                        bufferShow[i * bytesperline + j] = buffer[i * width * 3 + j];
                    }
                }
            }
            else if (pixSize == 2)
            {
                unsigned short *buffer_us = (unsigned short *)buffer;
                for (qint64 i = 0; i < height; i++)
                {
                    for (qint64 j = 0; j < width * 3; j++)
                    {
                        buffer_us[i * width * 3 + j] = ((buffer_us[i * width * 3 + j] & 0xff00) >> 8) | ((buffer_us[i * width * 3 + j] & 0x00ff) << 8);
                        bufferShow[i * bytesperline + j] = buffer_us[i * width * 3 + j] >> (bitDepth - 8);
                    }
                }
            }
        }

        rawDataPtr = nullptr;
        rawDataBit = 0;
        rawBayerType = RawFileInfoDlg::BayerPatternType::BAYER_UNKNOW;
        rawByteOrderType = RawFileInfoDlg::ByteOrderType::RAW_LITTLE_ENDIAN;
        yuvDataBit = 0;
        yuvDataPtr = nullptr;
        yuvType = YuvFileInfoDlg::YuvType::YUV_UNKNOW;
        openedImgType = PNM_IMG;
        pnmDataPtr = buffer;
        pnmDataBit = bitDepth;
        pgmDataPtr = nullptr;
        pgmDataBit = 0;

        resize(pixMap->size() * zoomList[zoomIdx]);
        repaint();
    }
    else
    {
        releaseBuffer();
        pixMap = new QImage(img);
        openedImgType = NORMAL_IMG;
        rawBayerType = RawFileInfoDlg::BayerPatternType::BAYER_UNKNOW;
        rawByteOrderType = RawFileInfoDlg::ByteOrderType::RAW_LITTLE_ENDIAN;
        rawDataBit = 0;
        rawDataPtr = nullptr;
        pnmDataBit = 0;
        pnmDataPtr = nullptr;
        pgmDataBit = 0;
        pgmDataPtr = nullptr;
        yuvDataBit = 0;
        yuvDataPtr = nullptr;
        yuvType = YuvFileInfoDlg::YuvType::YUV_UNKNOW;

        resize(pixMap->size() * zoomList[zoomIdx]);
        repaint();
    }
}

static void decompress10BitData(uint8_t *inputData, int width, int height, uint16_t *outputData)
{
    int inputSize = width * height * 10 / 8;
    // int outputSize = width * height;

    for (int i = 0, j = 0; i < inputSize; i += 5, j += 4)
    {
        outputData[j + 0] = (((uint16_t)(inputData[i + 1] & 0x03)) << 8) | (uint16_t)inputData[i + 0];
        outputData[j + 1] = (((uint16_t)(inputData[i + 2] & 0x0f)) << 6) | (uint16_t)((inputData[i + 1] & 0xfc) >> 2);
        outputData[j + 2] = (((uint16_t)(inputData[i + 3] & 0x3f)) << 4) | (uint16_t)((inputData[i + 2] & 0xf0) >> 4);
        outputData[j + 3] = (((uint16_t)(inputData[i + 4] & 0xff)) << 2) | (uint16_t)((inputData[i + 3] & 0xc0) >> 6);
    }
}

static void decompress12BitData(uint8_t *inputData, int width, int height, uint16_t *outputData)
{
    int inputSize = width * height * 12 / 8;
    // int outputSize = width * height;

    for (int i = 0, j = 0; i < inputSize; i += 3, j += 2)
    {
        outputData[j + 0] = (((uint16_t)(inputData[i + 1] & 0x0f)) << 8) | (uint16_t)inputData[i + 0];
        outputData[j + 1] = (((uint16_t)(inputData[i + 2] & 0xff)) << 4) | (uint16_t)((inputData[i + 1] & 0xf0) >> 4);
    }
}

static void decompress14BitData(uint8_t *inputData, int width, int height, uint16_t *outputData)
{
    int inputSize = width * height * 14 / 8;
    // int outputSize = width * height;

    for (int i = 0, j = 0; i < inputSize; i += 7, j += 4)
    {
        outputData[j + 0] = (((uint16_t)(inputData[i + 1] & 0x3f)) << 8) | (uint16_t)inputData[i + 0];
        outputData[j + 1] = (((uint16_t)(inputData[i + 3] & 0x0f)) << 10) | (((uint16_t)(inputData[i + 2] & 0xff)) << 2) | (uint16_t)((inputData[i + 1] & 0xc0) >> 6);
        outputData[j + 2] = (((uint16_t)(inputData[i + 5] & 0x03)) << 12) | (((uint16_t)(inputData[i + 4] & 0xff)) << 4) | (uint16_t)((inputData[i + 3] & 0xf0) >> 4);
        outputData[j + 4] = (((uint16_t)(inputData[i + 6] & 0xff)) << 6) | (uint16_t)((inputData[i + 5] & 0xfc) >> 2);
    }
}

void ImageWidget::setPixmap(QString &img, RawFileInfoDlg::BayerPatternType by, RawFileInfoDlg::ByteOrderType order, int bitDepth, bool compact, int width, int height) // raw
{
    int pixSize = 2;
    if (bitDepth <= 8)
    {
        pixSize = 1;
    }
    else if (bitDepth > 8 && bitDepth <= 16)
    {
        pixSize = 2;
    }
    else if (bitDepth > 16 && bitDepth <= 32)
    {
        pixSize = 4;
    }
    releaseBuffer();

    qint64 raw_buffer_length = pixSize * width * height;
    unsigned char *buffer = new unsigned char[raw_buffer_length];
    unsigned char *compact_buffer = compact ? new unsigned char[(bitDepth * width * height + 7) / 8] : nullptr;

    pixMap = new QImage(width, height, QImage::Format_Grayscale8);
    unsigned char *bufferShow = pixMap->bits();

    QFile rawFile(img);
    rawFile.open(QIODevice::ReadOnly);
    if (compact)
        rawFile.read((char *)compact_buffer, (bitDepth * width * height + 7) / 8);
    else
        rawFile.read((char *)buffer, raw_buffer_length);
    rawFile.close();

    if (compact) // 目前只有10,12,14 bit raw有compact
    {
        if (bitDepth != 10 && bitDepth != 12 && bitDepth != 14)
        {
            throw std::runtime_error("compact raw only support 10/12/14 bit!");
        }

        if (bitDepth == 10)
        {
            decompress10BitData(compact_buffer, width, height, (uint16_t *)buffer);
        }
        else if (bitDepth == 12)
        {
            decompress12BitData(compact_buffer, width, height, (uint16_t *)buffer);
        }
        else if (bitDepth == 14)
        {
            decompress14BitData(compact_buffer, width, height, (uint16_t *)buffer);
        }
        if (compact_buffer)
            delete[] compact_buffer;
    }

    int buffer_show_width = pixMap->bytesPerLine();
    if (pixSize == 1)
    {
        for (qint64 i = 0; i < height; i++)
        {
            for (qint64 j = 0; j < width; j++)
            {
                bufferShow[i * buffer_show_width + j] = buffer[i * width + j];
            }
        }
    }
    else if (pixSize == 2)
    {
        unsigned short *buffer_us = (unsigned short *)buffer;
        if (order == RawFileInfoDlg::RAW_BIG_ENDIAN)
        {
            for (qint64 i = 0; i < height; i++)
            {
                for (qint64 j = 0; j < width; j++)
                {
                    bufferShow[i * buffer_show_width + j] = CLIP3(((buffer_us[i * width + j] & 0x00ff << 8) | (buffer_us[i * width + j] & 0xff00 >> 8)) >> (bitDepth - 8), 0, 255);
                }
            }
        }
        else
        {
            for (qint64 i = 0; i < height; i++)
            {
                for (qint64 j = 0; j < width; j++)
                {
                    bufferShow[i * buffer_show_width + j] = CLIP3(buffer_us[i * width + j] >> (bitDepth - 8), 0, 255);
                }
            }
        }
    }
    else if (pixSize == 4)
    {
        unsigned int *buffer_ui = (unsigned int *)buffer;
        if (order == RawFileInfoDlg::RAW_BIG_ENDIAN)
        {
            for (qint64 i = 0; i < height; i++)
            {
                for (qint64 j = 0; j < width; j++)
                {
                    bufferShow[i * buffer_show_width + j] = CLIP3(((buffer_ui[i * width + j] & 0x000000ff << 24) | (buffer_ui[i * width + j] & 0x0000ff00 << 8) | (buffer_ui[i * width + j] & 0x00ff0000 >> 8) | (buffer_ui[i * width + j] & 0xff000000 >> 24)) >> (bitDepth - 8), 0, 255);
                }
            }
        }
        else
        {
            for (qint64 i = 0; i < height; i++)
            {
                for (qint64 j = 0; j < width; j++)
                {
                    bufferShow[i * buffer_show_width + j] = CLIP3(buffer_ui[i * width + j] >> (bitDepth - 8), 0, 255);
                }
            }
        }
    }

    rawDataPtr = buffer;
    rawDataBit = bitDepth;
    rawBayerType = by;
    rawByteOrderType = order;
    openedImgType = RAW_IMG;
    pnmDataPtr = nullptr;
    pnmDataBit = 0;
    pgmDataPtr = nullptr;
    pgmDataBit = 0;
    yuvDataBit = 0;
    yuvDataPtr = nullptr;
    yuvType = YuvFileInfoDlg::YuvType::YUV_UNKNOW;

    resize(pixMap->size() * zoomList[zoomIdx]);
    repaint();
}

static void convertYUV2RGB888(unsigned char *yuvBuf, unsigned char *rgb888Buf, int bitDepth, int width, int height, YuvFileInfoDlg::YuvType tp, int wholepixperline)
{
    if (tp == YuvFileInfoDlg::YuvType::YUV444_INTERLEAVE)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width * 3 + j * 3 + 0] >> (bitDepth - 8);
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[i * width * 3 + j * 3 + 1] >> (bitDepth - 8);
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[i * width * 3 + j * 3 + 2] >> (bitDepth - 8);
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
    else if (tp == YuvFileInfoDlg::YuvType::YUV444_PLANAR)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8);
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[width * height + i * width + j] >> (bitDepth - 8);
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[width * height * 2 + i * width + j] >> (bitDepth - 8);
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
    else if (tp == YuvFileInfoDlg::YuvType::YUV422_UYVY)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 1] >> (bitDepth - 8);
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2] >> (bitDepth - 8);     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 2] >> (bitDepth - 8); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2 - 2] >> (bitDepth - 8); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2] >> (bitDepth - 8);     // v
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
    else if (tp == YuvFileInfoDlg::YuvType::YUV422_YUYV)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2] >> (bitDepth - 8);
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 1] >> (bitDepth - 8); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 3] >> (bitDepth - 8); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2 - 1] >> (bitDepth - 8); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[i * width * 2 + j * 2 + 1] >> (bitDepth - 8); // v
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
    else if (tp == YuvFileInfoDlg::YuvType::YUV420_NV12)
    { // yyyyy...uvuv
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8);
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j] >> (bitDepth - 8);     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j + 1] >> (bitDepth - 8); // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j - 1] >> (bitDepth - 8); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j] >> (bitDepth - 8);     // v
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
    else if (tp == YuvFileInfoDlg::YuvType::YUV420_NV21)
    { // yyyyy...vuvu
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8);
                    if (j % 2 == 0)
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j + 1] >> (bitDepth - 8); // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j] >> (bitDepth - 8);     // v
                    }
                    else
                    {
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j] >> (bitDepth - 8);     // u
                        rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width + j - 1] >> (bitDepth - 8); // v
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
    else if (tp == YuvFileInfoDlg::YuvType::YUV420P_YU12)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8);
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8);                      // u
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[width * height + width * height / 4 + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8); // v
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
    else if (tp == YuvFileInfoDlg::YuvType::YUV420P_YV12)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8);
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = ((uint16_t *)yuvBuf)[width * height + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8);                      // v
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = ((uint16_t *)yuvBuf)[width * height + width * height / 4 + (i / 2) * width / 2 + j / 2] >> (bitDepth - 8); // u
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
    else if (tp == YuvFileInfoDlg::YuvType::YUV400)
    {
        if (bitDepth > 8 && bitDepth <= 16)
        {
            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = ((uint16_t *)yuvBuf)[i * width + j] >> (bitDepth - 8);
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

    if (tp != YuvFileInfoDlg::YuvType::YUV400)
    {
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                short y = rgb888Buf[i * wholepixperline * 3 + j * 3 + 0];
                short cb = (short)(rgb888Buf[i * wholepixperline * 3 + j * 3 + 1]) - 128;
                short cr = (short)(rgb888Buf[i * wholepixperline * 3 + j * 3 + 2]) - 128;
                short r = y + 1.403f * cr;
                short g = y - 0.714f * cr - 0.344f * cb;
                short b = y + 1.773f * cb;

                rgb888Buf[i * wholepixperline * 3 + j * 3 + 0] = CLIP3(r, 0, 255);
                rgb888Buf[i * wholepixperline * 3 + j * 3 + 1] = CLIP3(g, 0, 255);
                rgb888Buf[i * wholepixperline * 3 + j * 3 + 2] = CLIP3(b, 0, 255);
            }
        }
    }
}

void ImageWidget::setPixmap(QString &img, YuvFileInfoDlg::YuvType tp, int bitDepth, int width, int height, int pixSize) // yuv
{
    qint64 total_size = 0;

    if (tp == YuvFileInfoDlg::YUV444_INTERLEAVE || tp == YuvFileInfoDlg::YUV444_PLANAR)
    {
        total_size = pixSize * width * height * 3;
    }
    else if (tp == YuvFileInfoDlg::YUV422_UYVY || tp == YuvFileInfoDlg::YUV422_YUYV)
    {
        total_size = pixSize * width * height * 2;
    }
    else if (tp == YuvFileInfoDlg::YUV420_NV12 || tp == YuvFileInfoDlg::YUV420_NV21 || tp == YuvFileInfoDlg::YUV420P_YU12 || tp == YuvFileInfoDlg::YUV420P_YV12)
    {
        total_size = pixSize * width * height * 3 / 2;
    }
    else if (tp == YuvFileInfoDlg::YUV400)
    {
        total_size = pixSize * width * height;
    }

    releaseBuffer();

    unsigned char *buffer = new unsigned char[total_size];
    pixMap = new QImage(width, height, QImage::Format_RGB888);
    int entirpixperline = pixMap->bytesPerLine() / 3;
    unsigned char *bufferShow = pixMap->bits();

    QFile yuvFile(img);
    yuvFile.open(QIODevice::ReadOnly);
    yuvFile.read((char *)buffer, total_size);
    yuvFile.close();

    convertYUV2RGB888(buffer, bufferShow, bitDepth, width, height, tp, entirpixperline);

    rawDataPtr = nullptr;
    rawDataBit = 0;
    rawBayerType = RawFileInfoDlg::BAYER_UNKNOW;
    rawByteOrderType = RawFileInfoDlg::ByteOrderType::RAW_LITTLE_ENDIAN;
    openedImgType = YUV_IMG;
    pnmDataPtr = nullptr;
    pnmDataBit = 0;
    pgmDataPtr = nullptr;
    pgmDataBit = 0;
    yuvDataBit = bitDepth;
    yuvDataPtr = buffer;
    yuvType = tp;

    resize(pixMap->size() * zoomList[zoomIdx]);
    repaint();
}

void ImageWidget::zoomIn(int newZoomIdx)
{
    if (newZoomIdx <= ZOOM_LIST_LENGTH - 1)
    {
        float oldScaleRatio = ptCodInfo.originScaleRatio;
        this->zoomIdx = newZoomIdx;
        float scaleRatio = zoomList[this->zoomIdx];
        ptCodInfo.paintCoordinates[0] = ptCodInfo.originPaintCoordinates[0] * scaleRatio / oldScaleRatio;
        ptCodInfo.paintCoordinates[1] = ptCodInfo.originPaintCoordinates[1] * scaleRatio / oldScaleRatio;

        if (pixMap != nullptr)
        {
            QSize originSize = pixMap->size();
            resize(originSize * scaleRatio);
        }
    }
}

void ImageWidget::zoomOut(int newZoomIdx)
{
    if (newZoomIdx >= 0)
    {
        float oldScaleRatio = ptCodInfo.originScaleRatio;
        this->zoomIdx = newZoomIdx;
        float scaleRatio = zoomList[this->zoomIdx];
        ptCodInfo.paintCoordinates[0] = ptCodInfo.originPaintCoordinates[0] * scaleRatio / oldScaleRatio;
        ptCodInfo.paintCoordinates[1] = ptCodInfo.originPaintCoordinates[1] * scaleRatio / oldScaleRatio;

        if (pixMap != nullptr)
        {
            QSize originSize = pixMap->size();
            resize(originSize * scaleRatio);
        }
    }
}

void ImageWidget::processWheelZoom(QWheelEvent *event, int zoomDelta)
{
    int margin_left = 0, margin_top = 0, margin_right = 0, margin_bottom = 0;
    parentScroll->widget()->layout()->getContentsMargins(&margin_left, &margin_top, &margin_right, &margin_bottom);
    //    qDebug() << "old margin(l, t, r, b) " << margin_left << margin_top << margin_right << margin_bottom;

    int scrollAreaClientWidth = parentScroll->width() - parentScroll->horizontalScrollBar()->height();
    int scrollAreaClientHeight = parentScroll->height() - parentScroll->verticalScrollBar()->width();
    //    qDebug() << "old hv scroll client width/height " <<  scrollAreaClientWidth << scrollAreaClientHeight;

    //    qDebug() << "old hv scroll bar value " <<  parentScroll->horizontalScrollBar()->value() << parentScroll->verticalScrollBar()->value();
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QPointF img_pt = event->posF();
#else
    QPointF img_pt = event->position(); // imageWidget coordinate
#endif

    float oldScaleRatio = zoomList[zoomIdx];
    int pix_ind_x = img_pt.x() / oldScaleRatio; // real pix coordinate
    int pix_ind_y = img_pt.y() / oldScaleRatio;
    //    qDebug() << "pixel coordinate pos:" << pix_ind_x << pix_ind_y;

    int scrollArea_ind_x = img_pt.toPoint().x() + margin_left - parentScroll->horizontalScrollBar()->value(); // scroll area coordinate
    int scrollArea_ind_y = img_pt.toPoint().y() + margin_top - parentScroll->verticalScrollBar()->value();

    //    qDebug() << "scroll area coordinate pos:" << scrollArea_ind_x << scrollArea_ind_y;

    zoomIdx += zoomDelta;
    zoomIdx = zoomIdx < 0 ? 0 : zoomIdx;
    zoomIdx = zoomIdx > ZOOM_LIST_LENGTH - 1 ? ZOOM_LIST_LENGTH - 1 : zoomIdx;
    float scaleRatio = zoomList[zoomIdx];
    float originPtSclRatio = ptCodInfo.originScaleRatio;
    ptCodInfo.paintCoordinates[0] = ptCodInfo.originPaintCoordinates[0] * scaleRatio / originPtSclRatio;
    ptCodInfo.paintCoordinates[1] = ptCodInfo.originPaintCoordinates[1] * scaleRatio / originPtSclRatio;

    QSize originSize = pixMap->size();

    int new_img_pt_x = pix_ind_x * scaleRatio; // img widget coordinate
    int new_img_pt_y = pix_ind_y * scaleRatio;

    int new_container_x = new_img_pt_x + margin_left; // container coordinate
    int new_container_y = new_img_pt_y + margin_top;

    //    qDebug() << "new container coordinate pos:" << new_container_x << new_container_y;

    int hscrollbar_value = new_container_x - scrollArea_ind_x;
    int vscrollbar_value = new_container_y - scrollArea_ind_y;

    if (hscrollbar_value < 0)
    {
        margin_left = margin_left - hscrollbar_value;
        hscrollbar_value = 0;
    }
    if (vscrollbar_value < 0)
    {
        margin_top = margin_top - vscrollbar_value;
        vscrollbar_value = 0;
    }

    //    qDebug() << "h v scrollbar: " << hscrollbar_value << vscrollbar_value;

    if (originSize.height() * scaleRatio + margin_top + margin_bottom < scrollAreaClientHeight)
    {
        margin_bottom = scrollAreaClientHeight - originSize.height() * scaleRatio - margin_top;
    }
    if (originSize.width() * scaleRatio + margin_left + margin_right < scrollAreaClientWidth)
    {
        margin_right = scrollAreaClientWidth - originSize.width() * scaleRatio - margin_left;
    }

    if (originSize.height() * scaleRatio + margin_top + margin_bottom > scrollAreaClientHeight)
    {
        margin_bottom = scrollAreaClientHeight - originSize.height() * scaleRatio - margin_top;
        margin_bottom = margin_bottom < 0 ? 0 : margin_bottom;
    }
    if (margin_left + margin_right + originSize.width() * scaleRatio > scrollAreaClientWidth)
    {
        margin_right = scrollAreaClientWidth - originSize.width() * scaleRatio - margin_left;
        margin_right = margin_right < 0 ? 0 : margin_right;
    }

    //    qDebug() << "new margin(l, t, r, b) " << margin_left << margin_top << margin_right << margin_bottom;
    if (vscrollbar_value > 0 && (vscrollbar_value + scrollAreaClientHeight) > (originSize.height() * scaleRatio + margin_top + margin_bottom))
    {
        margin_bottom = (vscrollbar_value + scrollAreaClientHeight) - (originSize.height() * scaleRatio + margin_top);
    }
    if (hscrollbar_value > 0 && (hscrollbar_value + scrollAreaClientWidth) > (originSize.width() * scaleRatio + margin_left + margin_right))
    {
        margin_right = (hscrollbar_value + scrollAreaClientWidth) - (originSize.width() * scaleRatio + margin_left);
    }

    if (hscrollbar_value > margin_left && originSize.width() * scaleRatio > scrollAreaClientWidth)
    {
        hscrollbar_value -= margin_left;
        margin_left = 0;
    }
    if (vscrollbar_value > margin_top && originSize.height() * scaleRatio > scrollAreaClientHeight)
    {
        vscrollbar_value -= margin_top;
        margin_top = 0;
    }

    parentScroll->widget()->resize(originSize * scaleRatio + QSize(margin_left + margin_right, margin_top + margin_bottom));
    parentScroll->widget()->layout()->setContentsMargins(margin_left, margin_top, margin_right, margin_bottom);
    parentScroll->horizontalScrollBar()->setValue(hscrollbar_value);
    parentScroll->verticalScrollBar()->setValue(vscrollbar_value);

    resize(originSize * scaleRatio);

    if (zoomDelta > 0)
    {
        emit inform_zoom_in(zoomIdx);
    }
    else
    {
        emit inform_zoom_out(zoomIdx);
    }
}

void ImageWidget::releaseBuffer()
{
    if (pixMap != nullptr)
    {
        delete pixMap;
        pixMap = nullptr;
    }
    if (rawDataPtr != nullptr)
    {
        delete[] rawDataPtr;
        rawDataPtr = nullptr;
    }
    if (pnmDataPtr != nullptr)
    {
        delete[] pnmDataPtr;
        pnmDataPtr = nullptr;
    }
    if (pgmDataPtr != nullptr)
    {
        delete[] pgmDataPtr;
        pgmDataPtr = nullptr;
    }
    if (yuvDataPtr != nullptr)
    {
        delete[] yuvDataPtr;
        yuvDataPtr = nullptr;
    }
}

void ImageWidget::acceptImgFromOther(const ImageWidget *other)
{
    pixMapBak = pixMap;
    rawDataPtrBak = rawDataPtr;
    rawDataBitBak = rawDataBit;
    pnmDataPtrBak = pnmDataPtr;
    pnmDataBitBak = pnmDataBit;
    pgmDataPtrBak = pgmDataPtr;
    pgmDataBitBak = pgmDataBit;
    yuvDataPtrBak = yuvDataPtr;
    yuvDataBitBak = yuvDataBit;
    rawBayerTypeBak = rawBayerType;
    rawByteOrderTypeBak = rawByteOrderType;
    yuvTypeBak = yuvType;
    openedImgTypeBak = openedImgType;

    pixMap = other->pixMap;
    rawDataPtr = other->rawDataPtr;
    rawDataBit = other->rawDataBit;
    pnmDataPtr = other->pnmDataPtr;
    pnmDataBit = other->pnmDataBit;
    pgmDataPtr = other->pgmDataPtr;
    pgmDataBit = other->pgmDataBit;
    yuvDataPtr = other->yuvDataPtr;
    yuvDataBit = other->yuvDataBit;
    rawBayerType = other->rawBayerType;
    rawByteOrderType = other->rawByteOrderType;
    yuvType = other->yuvType;
    openedImgType = other->openedImgType;
    update();
}

void ImageWidget::restoreImg()
{
    pixMap = pixMapBak;
    rawDataPtr = rawDataPtrBak;
    rawDataBit = rawDataBitBak;
    pnmDataPtr = pnmDataPtrBak;
    pnmDataBit = pnmDataBitBak;
    pgmDataPtr = pgmDataPtrBak;
    pgmDataBit = pgmDataBitBak;
    yuvDataPtr = yuvDataPtrBak;
    yuvDataBit = yuvDataBitBak;
    rawBayerType = rawBayerTypeBak;
    rawByteOrderType = rawByteOrderTypeBak;
    yuvType = yuvTypeBak;
    openedImgType = openedImgTypeBak;
    update();
}
