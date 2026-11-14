#include "ImgInfoDlg.h"
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QAbstractTextDocumentLayout>
#include <QResizeEvent>
#include <QTextDocument>
#include <QTextOption>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
class WrappingTextEdit final : public QTextEdit
{
public:
    explicit WrappingTextEdit(QWidget *parent) : QTextEdit(parent)
    {
        setReadOnly(true);
        setFrameStyle(QFrame::NoFrame);
        setLineWrapMode(QTextEdit::WidgetWidth);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        document()->setDocumentMargin(0);
        QTextOption textOption = document()->defaultTextOption();
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        document()->setDefaultTextOption(textOption);
        connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this,
            [this](const QSizeF &)
            {
                updateHeight();
            });
        updateHeight();
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QTextEdit::resizeEvent(event);
        updateHeight();
    }

private:
    void updateHeight()
    {
        const int contentHeight = qCeil(document()->size().height());
        setFixedHeight(qMax(contentHeight, fontMetrics().height()));
    }
};

QString bayerPatternName(BayerPatternType pattern)
{
    switch (pattern)
    {
    case BayerPatternType::RGGB:
        return QStringLiteral("RGGB");
    case BayerPatternType::GRBG:
        return QStringLiteral("GRBG");
    case BayerPatternType::GBRG:
        return QStringLiteral("GBRG");
    case BayerPatternType::BGGR:
        return QStringLiteral("BGGR");
    case BayerPatternType::RGGIR:
        return QStringLiteral("RGGIR");
    case BayerPatternType::BGGIR:
        return QStringLiteral("BGGIR");
    case BayerPatternType::GRIRG:
        return QStringLiteral("GRIRG");
    case BayerPatternType::GBIRG:
        return QStringLiteral("GBIRG");
    case BayerPatternType::GIRRG:
        return QStringLiteral("GIRRG");
    case BayerPatternType::GIRBG:
        return QStringLiteral("GIRBG");
    case BayerPatternType::IRGGR:
        return QStringLiteral("IRGGR");
    case BayerPatternType::IRGGB:
        return QStringLiteral("IRGGB");
    case BayerPatternType::MONO:
        return QStringLiteral("MONO");
    case BayerPatternType::BAYER_UNKNOW:
        return QStringLiteral("-");
    }
    return QStringLiteral("-");
}

QString yuvFormatName(YuvType format)
{
    switch (format)
    {
    case YuvType::YUV444_INTERLEAVE:
        return QStringLiteral("YUV444_INTERLEAVE");
    case YuvType::YUV444_PLANAR:
        return QStringLiteral("YUV444_PLANAR");
    case YuvType::YUV422_UYVY:
        return QStringLiteral("YUV422_UYVY");
    case YuvType::YUV422_YUYV:
        return QStringLiteral("YUV422_YUYV");
    case YuvType::YUV420_NV12:
        return QStringLiteral("YUV420_NV12");
    case YuvType::YUV420_NV21:
        return QStringLiteral("YUV420_NV21");
    case YuvType::YUV420P_YU12:
        return QStringLiteral("YUV420P_YU12");
    case YuvType::YUV420P_YV12:
        return QStringLiteral("YUV420P_YV12");
    case YuvType::YUV400:
        return QStringLiteral("YUV400");
    case YuvType::YUV422_YUYV_AS1:
        return QStringLiteral("YUV422_YUYV_AS1");
    case YuvType::YUV422_UYVY_AS1:
        return QStringLiteral("YUV422_UYVY_AS1");
    case YuvType::YUV_UNKNOW:
        return QStringLiteral("-");
    }
    return QStringLiteral("-");
}
}

ImgInfoDlg::ImgInfoDlg(QWidget *parent) : QDialog(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    layout->addWidget(createImageCard(tr("Left image"), leftCard));
    layout->addWidget(createImageCard(tr("Right image"), rightCard));

    setWindowTitle(tr("image info"));
    setMinimumSize(620, 250);
    resize(800, 300); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
}

QWidget *ImgInfoDlg::createImageCard(const QString &title, ImageInfoCard &card)
{
    QFrame *frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("imageInfoCard"));
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setStyleSheet(QStringLiteral(
        "QFrame#imageInfoCard { border: 1px solid palette(mid); border-radius: 6px; }"
        "QLabel#imageInfoTitle { font-weight: 600; font-size: 15px; border: none; }"
        "QLabel#imageInfoFileName { font-weight: 600; border: none; }"
        "QLabel { border: none; }"));

    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(14, 12, 14, 14);
    layout->setSpacing(8);

    QLabel *titleLabel = new QLabel(title, frame);
    titleLabel->setObjectName(QStringLiteral("imageInfoTitle"));
    layout->addWidget(titleLabel);

    QFrame *separator = new QFrame(frame);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    QGridLayout *infoLayout = new QGridLayout();
    infoLayout->setHorizontalSpacing(12);
    infoLayout->setVerticalSpacing(8);
    infoLayout->setColumnStretch(1, 1);

    auto addRow = [frame, infoLayout](int row, const QString &labelText, QWidget *valueWidget, const QString &objectName = QString())
    {
        QLabel *fieldLabel = new QLabel(labelText, frame);
        fieldLabel->setStyleSheet(QStringLiteral("color: palette(mid);"));
        if (!objectName.isEmpty())
        {
            valueWidget->setObjectName(objectName);
        }
        infoLayout->addWidget(fieldLabel, row, 0, Qt::AlignTop);
        infoLayout->addWidget(valueWidget, row, 1);
    };

    card.fileName = new WrappingTextEdit(frame);
    card.fileName->setPlainText(QStringLiteral("-"));
    card.fileName->setStyleSheet(QStringLiteral("font-weight: 600; background: transparent;"));
    addRow(0, tr("File"), card.fileName, QStringLiteral("imageInfoFileName"));
    card.filePath = new WrappingTextEdit(frame);
    card.filePath->setPlainText(QStringLiteral("-"));
    card.filePath->setStyleSheet(QStringLiteral("background: transparent;"));
    addRow(1, tr("Location"), card.filePath);
    card.dimensions = new QLabel(QStringLiteral("-"), frame);
    card.dimensions->setTextInteractionFlags(Qt::TextSelectableByMouse);
    addRow(2, tr("Dimensions"), card.dimensions);
    card.bitDepth = new QLabel(QStringLiteral("-"), frame);
    card.bitDepth->setTextInteractionFlags(Qt::TextSelectableByMouse);
    addRow(3, tr("Bit depth"), card.bitDepth);
    card.bayerPattern = new QLabel(QStringLiteral("-"), frame);
    card.bayerPattern->setTextInteractionFlags(Qt::TextSelectableByMouse);
    addRow(4, tr("Bayer pattern"), card.bayerPattern);
    card.yuvFormat = new QLabel(QStringLiteral("-"), frame);
    card.yuvFormat->setTextInteractionFlags(Qt::TextSelectableByMouse);
    addRow(5, tr("YUV format"), card.yuvFormat);

    infoLayout->setRowStretch(6, 1);
    layout->addLayout(infoLayout, 1);
    layout->addStretch(1);
    return frame;
}

void ImgInfoDlg::setImgInfo(const QString &filePath, const QSize &imgSize, BayerPatternType bay, YuvType yuv_tp, int bits, bool left)
{
    ImageInfoCard &card = left ? leftCard : rightCard;
    const QFileInfo fileInfo(filePath);
    card.fileName->setPlainText(fileInfo.fileName());
    card.fileName->setToolTip(filePath);
    card.filePath->setPlainText(fileInfo.path());
    card.filePath->setToolTip(fileInfo.path());
    card.dimensions->setText(QStringLiteral("%1 × %2").arg(imgSize.width()).arg(imgSize.height()));
    card.bitDepth->setText(tr("%1 bit").arg(bits));
    card.bayerPattern->setText(bayerPatternName(bay));
    card.yuvFormat->setText(yuvFormatName(yuv_tp));
}
