#include "ImgInfoDlg.h"
#include <QHBoxLayout>

ImgInfoDlg::ImgInfoDlg(QWidget *parent) : QDialog(parent)
{
    leftLabel = new QLabel(this);
    rightLabel = new QLabel(this);
    QFrame *centerLine = new QFrame();
    centerLine->setFrameShape(QFrame::Shape::VLine);
    centerLine->setFrameShadow(QFrame::Shadow::Sunken);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(leftLabel);
    layout->addWidget(centerLine);
    layout->addWidget(rightLabel);
    setLayout(layout);
    resize(400, 200);
    setWindowTitle(tr("image info"));
}

void ImgInfoDlg::setImgInfo(QString &filePath, QSize &imgSize, BayerPatternType by, YuvType yt, int bits, bool left)
{
    // if (filePath.length() == 0) {
    //     if (left) {
    //         leftLabel->setText("no input");
    //     } else {
    //         rightLabel->setText("no input");
    //     }
    //     return;
    // }
    QString tmp = QString::asprintf("\nw: %d, h: %d, %d bit\n", imgSize.width(), imgSize.height(), bits);
    if (by == BayerPatternType::RGGB)
    {
        tmp += QString(" RGGB");
    }
    else if (by == BayerPatternType::GRBG)
    {
        tmp += QString(" GRBG");
    }
    else if (by == BayerPatternType::GBRG)
    {
        tmp += QString(" GBRG");
    }
    else if (by == BayerPatternType::BGGR)
    {
        tmp += QString(" BGGR");
    }
    if (yt == YuvType::YUV444_INTERLEAVE)
    {
        tmp += QString(" YUV444_INTERLEAVE");
    }
    else if (yt == YuvType::YUV444_PLANAR)
    {
        tmp += QString(" YUV444_PLANAR");
    }
    else if (yt == YuvType::YUV422_UYVY)
    {
        tmp += QString(" YUV422_UYVY");
    }
    else if (yt == YuvType::YUV422_YUYV)
    {
        tmp += QString(" YUV422_YUYV");
    }
    else if (yt == YuvType::YUV420_NV12)
    {
        tmp += QString(" YUV420_NV12");
    }
    else if (yt == YuvType::YUV420_NV21)
    {
        tmp += QString(" YUV420_NV21");
    }
    else if (yt == YuvType::YUV420P_YU12)
    {
        tmp += QString(" YUV420P_YU12");
    }
    else if (yt == YuvType::YUV420P_YV12)
    {
        tmp += QString(" YUV420P_YV12");
    }
    else if (yt == YuvType::YUV400)
    {
        tmp += QString(" YUV400");
    }
    if (left)
    {
        leftLabel->setText(filePath + tmp);
    }
    else
    {
        rightLabel->setText(filePath + tmp);
    }
}
