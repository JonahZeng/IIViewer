#include "CurveAdjustDialog.h"
// #include "ImageWidget.h"
#include "ColorSpaceCvt.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QDebug>
#include <QtMath>
#include <QtCharts/QValueAxis>
#include <QGraphicsSceneEvent>


CurveAdjustChart::CurveAdjustChart(quint16 srcImgBits) : QChart(), m_currentSpSeries(nullptr), m_currentScaSeries(nullptr)
{
    m_selectedPointIdx = -1;
    m_dragging = false;
    m_srcImgBits = srcImgBits;

    initUI();
}

void CurveAdjustChart::initUI()
{
    quint32 x_max = (1u << m_srcImgBits) - 1;
    // 创建坐标轴
    QValueAxis* m_axisX = new QValueAxis();
    QValueAxis* m_axisY = new QValueAxis();
    m_axisX->setRange(0, x_max);
    m_axisY->setRange(0, 255);
    m_axisX->setTitleText(tr("Input"));
    m_axisY->setTitleText(tr("Output"));
    m_axisX->setLabelFormat("%d");
    m_axisY->setLabelFormat("%d");
    addAxis(m_axisX, Qt::AlignmentFlag::AlignBottom);
    addAxis(m_axisY, Qt::AlignmentFlag::AlignLeft);

    m_currentSpSeries = new QSplineSeries();
    m_currentScaSeries = new QScatterSeries();
    addSeries(m_currentSpSeries);
    addSeries(m_currentScaSeries);

    m_currentSpSeries->attachAxis(m_axisX);
    m_currentSpSeries->attachAxis(m_axisY);
    m_currentScaSeries->attachAxis(m_axisX);
    m_currentScaSeries->attachAxis(m_axisY);

    m_currentScaSeries->setMarkerSize(12);
    m_currentScaSeries->setMarkerShape(QScatterSeries::MarkerShape::MarkerShapeCircle);
    // 设置散点系列可点击
    m_currentScaSeries->setPointsVisible(true);

    connect(m_currentScaSeries, &QScatterSeries::pressed, this, &CurveAdjustChart::pointSelected);
}

CurveAdjustChart::~CurveAdjustChart()
{

}

void CurveAdjustChart::pointSelected(QPointF pst)
{
    int k = 0;
    for(QPointF& pt: m_currentScaSeries->points())
    {
        if(pt == pst)
        {
            m_selectedPointIdx = k;
            m_dragging = true;
            grabMouse();
            break;
        }
        k += 1;
    }
}

void CurveAdjustChart::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
}

void CurveAdjustChart::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(m_dragging && event->button() == Qt::MouseButton::LeftButton)
    {
        ungrabMouse();
        m_selectedPointIdx = -1;
        m_dragging = false;
    }
}

void CurveAdjustChart::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(m_dragging)
    {
        QPointF new_pos = mapToValue(event->pos(), m_currentScaSeries);
        auto old_x = m_currentScaSeries->at(m_selectedPointIdx).x();
        new_pos.setX(old_x);
        auto new_y = new_pos.y();
        new_pos.setY(CLIP3(new_y, 0.0, 255.0));
        m_currentScaSeries->replace(m_selectedPointIdx, new_pos);
        m_currentSpSeries->replace(m_selectedPointIdx, new_pos);
    }
}

CurveAdjustDialog::CurveAdjustDialog(QImage *disp_image, void* src_img, OpenedImageType img_type, quint16 src_img_bits, const std::array<QPoint, 9>& preset_curve, QWidget *parent)
    : QDialog(parent)
    , m_originalImage(src_img)
    , m_originImgType(img_type)
    , m_dispImage(disp_image)
    , m_yuvType(YuvType::YUV_UNKNOW)
    , m_chartView(nullptr)
    , m_chart(nullptr)
    , m_rgbCurve(preset_curve)
    , m_srcImgBits(src_img_bits)
{
    setModal(false); // 设置为非模态对话框
    setWindowTitle(tr("Curve Adjustment"));
    setMinimumSize(600, 600);
    initUI();

    // Calculate spline parameters from preset_curve and populate srcToDispLut
    calculateSplineLutFromCurve();
}

CurveAdjustDialog::~CurveAdjustDialog()
{
}

void CurveAdjustDialog::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_chartView = new QChartView(this);
    m_chart = new CurveAdjustChart(m_srcImgBits);
    m_chart->setTitle(tr("Curve Adjustment"));
    m_chart->legend()->hide();
    m_chartView->setChart(m_chart);
    m_chartView->setRenderHint(QPainter::RenderHint::Antialiasing);

    mainLayout->addWidget(m_chartView);
    
    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_resetButton = new QPushButton(tr("Reset"));
    m_applyButton = new QPushButton(tr("Apply"));
    m_cancelButton = new QPushButton(tr("Cancel"));
    
    connect(m_resetButton, &QPushButton::clicked, this, &CurveAdjustDialog::onResetClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &CurveAdjustDialog::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &CurveAdjustDialog::onCancelClicked);
    
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);

    m_applyButton->setFocus();
    
    // 更新图表显示
    updateChartFromCurve();
}

void CurveAdjustDialog::updateChartFromCurve()
{
    QColor curveColor = Qt::GlobalColor::black;

    m_chart->m_currentSpSeries->clear();
    m_chart->m_currentScaSeries->clear();
    for (const QPoint& point : m_rgbCurve) {
        m_chart->m_currentSpSeries->append(point.x(), point.y());
        m_chart->m_currentScaSeries->append(point.x(), point.y());
    }
    
    QPen pen(curveColor);
    pen.setWidth(2);
    m_chart->m_currentSpSeries->setPen(pen);
    m_chart->m_currentScaSeries->setColor(curveColor);
}

void CurveAdjustDialog::updateCurveFromChart()
{
    for (int i = 0; i < 9; i++)
    {
        m_rgbCurve[i].setY(m_chart->m_currentSpSeries->at(i).y());
    }
}

void CurveAdjustDialog::applyCurveToImage()
{
    // 1. 根据originImgType和m_srcImgBits，转换m_originalImage类型
    // 2. 获取m_dispImage size
    // 3. 根据m_rgbCurve插值出完整的x-->y映射lut
    // 4. 像素索引x获得y
    int img_w = m_dispImage->width();
    int img_h = m_dispImage->height();
    calculateSplineLutFromCurve();

    if (m_originImgType == OpenedImageType::NORMAL_IMG)
    {
        QImage *src_img = reinterpret_cast<QImage *>(m_originalImage);
        for (int y = 0; y < img_h; y++)
        {
            for (int x = 0; x < img_w; x++)
            {
                QRgb rgb = src_img->pixel(x, y);
                m_dispImage->setPixel(x, y, qRgb(srcToDispLut[qRed(rgb)], srcToDispLut[qGreen(rgb)], srcToDispLut[qBlue(rgb)]));
            }
        }
    }
    else if (m_originImgType == OpenedImageType::PGM_IMG && m_srcImgBits <= 8)
    {
        uint8_t *src_img = reinterpret_cast<uint8_t *>(m_originalImage);
        for (int y = 0; y < img_h; y++)
        {
            for (int x = 0; x < img_w; x++)
            {
                uint8_t gray = src_img[y * img_w + x];
                m_dispImage->setPixel(x, y, qRgb(srcToDispLut[gray], srcToDispLut[gray], srcToDispLut[gray]));
            }
        }
    }
    else if(m_originImgType == OpenedImageType::PGM_IMG && m_srcImgBits > 8)
    {
        uint16_t step = 1 << (m_srcImgBits - 8);
        uint16_t* src_img = reinterpret_cast<uint16_t*>(m_originalImage);
        for (int y = 0; y < img_h; y++)
        {
            for (int x = 0; x < img_w; x++)
            {
                uint16_t gray = src_img[y * img_w + x];
                uint16_t idx0 = gray >> (m_srcImgBits - 8);
                uint16_t idx1 = qMin(idx0 + 1, 256);
                uint16_t delta = gray - (idx0 << (m_srcImgBits - 8));

                uint8_t out_gray = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));
                m_dispImage->setPixel(x, y, qRgb(out_gray, out_gray, out_gray));
            }
        }
    }
    else if(m_originImgType == OpenedImageType::PNM_IMG && m_srcImgBits <= 8)
    {
        uint8_t* src_img = reinterpret_cast<uint8_t*>(m_originalImage);
        for (int y = 0; y < img_h; y++)
        {
            for (int x = 0, x_j = 0; x < img_w * 3; x++, x_j++)
            {
                uint8_t red = src_img[y * img_w * 3 + x];
                uint8_t green = src_img[y * img_w * 3 + x + 1];
                uint8_t blue = src_img[y * img_w * 3 + x + 2];
                m_dispImage->setPixel(x_j, y, qRgb(srcToDispLut[red], srcToDispLut[green], srcToDispLut[blue]));
            }
        }
    }
    else if(m_originImgType == OpenedImageType::PNM_IMG && m_srcImgBits <= 16)
    {
        uint16_t step = 1 << (m_srcImgBits - 8);
        uint16_t idx0 = 0, idx1 = 0, delta = 0;
        uint8_t out_red = 0, out_green = 0, out_blue = 0;
        uint16_t* src_img = reinterpret_cast<uint16_t*>(m_originalImage);
        for (int y = 0; y < img_h; y++)
        {
            for (int x = 0, x_j = 0; x < img_w * 3; x += 3, x_j += 1)
            {
                uint16_t red = src_img[y * img_w * 3 + x];
                idx0 = red >> (m_srcImgBits - 8);
                idx1 = qMin(idx0 + 1, 256);
                delta = red - (idx0 << (m_srcImgBits - 8));
                out_red = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));

                uint16_t green = src_img[y * img_w * 3 + x + 1];
                idx0 = green >> (m_srcImgBits - 8);
                idx1 = qMin(idx0 + 1, 256);
                delta = green - (idx0 << (m_srcImgBits - 8));
                out_green = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));

                uint16_t blue = src_img[y * img_w * 3 + x + 2];
                idx0 = blue >> (m_srcImgBits - 8);
                idx1 = qMin(idx0 + 1, 256);
                delta = blue - (idx0 << (m_srcImgBits - 8));
                out_blue = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));

                m_dispImage->setPixel(x_j, y, qRgb(out_red, out_green, out_blue));
            }
        }
    }
    else if(m_originImgType == OpenedImageType::RAW_IMG && m_srcImgBits <= 8)
    {
        uint8_t* src_img = reinterpret_cast<uint8_t*>(m_originalImage);
        for (int y = 0; y < img_h; y++)
        {
            for (int x = 0; x < img_w; x++)
            {
                uint8_t gray = src_img[y * img_w + x];
                m_dispImage->setPixel(x, y, qRgb(srcToDispLut[gray], srcToDispLut[gray], srcToDispLut[gray]));
            }
        }
    }
    else if(m_originImgType == OpenedImageType::RAW_IMG && m_srcImgBits <= 16)
    {
        uint16_t step = 1 << (m_srcImgBits - 8);
        uint16_t* src_img = reinterpret_cast<uint16_t*>(m_originalImage);
        for (int y = 0; y < img_h; y++)
        {
            for (int x = 0; x < img_w; x++)
            {
                uint16_t gray = src_img[y * img_w + x];
                uint16_t idx0 = gray >> (m_srcImgBits - 8);
                uint16_t idx1 = qMin(idx0 + 1, 256);
                uint16_t delta = gray - (idx0 << (m_srcImgBits - 8));

                uint8_t out_gray = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));
                m_dispImage->setPixel(x, y, qRgb(out_gray, out_gray, out_gray));
            }
        }
    }
    else if(m_originImgType == OpenedImageType::RAW_IMG && m_srcImgBits <= 28)
    {
        uint32_t step = 1 << (m_srcImgBits - 8);
        uint32_t* src_img = reinterpret_cast<uint32_t*>(m_originalImage);
        for (int y = 0; y < img_h; y++)
        {
            for (int x = 0; x < img_w; x++)
            {
                uint32_t gray = src_img[y * img_w + x];
                uint32_t idx0 = gray >> (m_srcImgBits - 8);
                uint32_t idx1 = qMin(idx0 + 1u, 256u);
                uint32_t delta = gray - (idx0 << (m_srcImgBits - 8));

                uint8_t out_gray = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));
                m_dispImage->setPixel(x, y, qRgb(out_gray, out_gray, out_gray));
            }
        }
    }
    else if(m_originImgType == OpenedImageType::YUV_IMG && m_srcImgBits <= 8)
    {
        uint8_t* src_img = reinterpret_cast<uint8_t*>(m_originalImage);
        if(m_yuvType == YuvType::YUV400)
        {
            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w; x++)
                {
                    uint8_t gray = src_img[y * img_w + x];
                    m_dispImage->setPixel(x, y, qRgb(srcToDispLut[gray], srcToDispLut[gray], srcToDispLut[gray]));
                }
            }
        }
        else if(m_yuvType == YuvType::YUV420_NV12 || m_yuvType == YuvType::YUV420_NV21 || m_yuvType == YuvType::YUV420P_YU12 || m_yuvType == YuvType::YUV420P_YV12)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 3 / 2]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w; x++)
                {
                    yuv_buf_8b[y * img_w + x] = srcToDispLut[src_img[y * img_w + x]];
                }
            }

            for (int y = 0; y < img_h / 2; y++)
            {
                for (int x = 0; x < img_w; x++)
                {
                    yuv_buf_8b[img_h * img_w + y * img_w + x] = src_img[img_h * img_w + y * img_w + x] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
        else if(m_yuvType == YuvType::YUV422_UYVY)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 2]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 1; x < img_w * 2; x += 2)
                {
                    yuv_buf_8b[y * img_w * 2 + x] = srcToDispLut[src_img[y * img_w * 2 + x]];
                }
            }

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w * 2; x += 2)
                {
                    yuv_buf_8b[y * img_w * 2 + x] = src_img[y * img_w * 2 + x] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
        else if(m_yuvType == YuvType::YUV422_YUYV)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 2]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w * 2; x += 2)
                {
                    yuv_buf_8b[y * img_w * 2 + x] = srcToDispLut[src_img[y * img_w * 2 + x]];
                }
            }

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 1; x < img_w * 2; x += 2)
                {
                    yuv_buf_8b[y * img_w * 2 + x] = src_img[y * img_w * 2 + x] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
        else if(m_yuvType == YuvType::YUV444_INTERLEAVE)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 3]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w * 3; x += 3)
                {
                    yuv_buf_8b[y * img_w * 3 + x] = srcToDispLut[src_img[y * img_w * 3 + x]];
                    yuv_buf_8b[y * img_w * 3 + x + 1] = src_img[y * img_w * 3 + x + 1] >> (m_srcImgBits - 8);
                    yuv_buf_8b[y * img_w * 3 + x + 2] = src_img[y * img_w * 3 + x + 2] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
        else if(m_yuvType == YuvType::YUV444_PLANAR)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 3]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w; x += 1)
                {
                    yuv_buf_8b[y * img_w + x] = srcToDispLut[src_img[y * img_w + x]];
                }
            }

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w * 2; x++)
                {
                    yuv_buf_8b[img_h * img_w + y * img_w * 2 + x] = src_img[img_h * img_w + y * img_w * 2 + x] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
    }
    else if(m_originImgType == OpenedImageType::YUV_IMG && m_srcImgBits <= 16)
    {
        uint16_t step = 1 << (m_srcImgBits - 8);
        uint16_t* src_img = reinterpret_cast<uint16_t*>(m_originalImage);
        if(m_yuvType == YuvType::YUV400)
        {
            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w; x++)
                {
                    uint16_t gray = src_img[y * img_w + x];
                    uint16_t idx0 = gray >> (m_srcImgBits - 8);
                    uint16_t idx1 = qMin(idx0 + 1, 256);
                    uint16_t delta = gray - (idx0 << (m_srcImgBits - 8));

                    uint8_t out_gray = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));
                    m_dispImage->setPixel(x, y, qRgb(out_gray, out_gray, out_gray));
                }
            }
        }
        else if(m_yuvType == YuvType::YUV420_NV12 || m_yuvType == YuvType::YUV420_NV21 || m_yuvType == YuvType::YUV420P_YU12 || m_yuvType == YuvType::YUV420P_YV12)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 3 / 2]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w; x++)
                {
                    uint16_t gray = src_img[y * img_w + x];
                    uint16_t idx0 = gray >> (m_srcImgBits - 8);
                    uint16_t idx1 = qMin(idx0 + 1, 256);
                    uint16_t delta = gray - (idx0 << (m_srcImgBits - 8));
                    yuv_buf_8b[y * img_w + x] = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));
                }
            }

            for (int y = 0; y < img_h / 2; y++)
            {
                for (int x = 0; x < img_w; x++)
                {
                    yuv_buf_8b[img_h * img_w + y * img_w + x] = src_img[img_h * img_w + y * img_w + x] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
        else if(m_yuvType == YuvType::YUV422_UYVY)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 2]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 1; x < img_w * 2; x += 2)
                {
                    uint16_t gray = src_img[y * img_w * 2 + x];
                    uint16_t idx0 = gray >> (m_srcImgBits - 8);
                    uint16_t idx1 = qMin(idx0 + 1, 256);
                    uint16_t delta = gray - (idx0 << (m_srcImgBits - 8));
                    yuv_buf_8b[y * img_w * 2 + x] = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));
                }
            }

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w * 2; x += 2)
                {
                    yuv_buf_8b[y * img_w * 2 + x] = src_img[y * img_w * 2 + x] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
        else if(m_yuvType == YuvType::YUV422_YUYV)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 2]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w * 2; x += 2)
                {
                    uint16_t gray = src_img[y * img_w * 2 + x];
                    uint16_t idx0 = gray >> (m_srcImgBits - 8);
                    uint16_t idx1 = qMin(idx0 + 1, 256);
                    uint16_t delta = gray - (idx0 << (m_srcImgBits - 8));
                    yuv_buf_8b[y * img_w * 2 + x] = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));
                }
            }

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 1; x < img_w * 2; x += 2)
                {
                    yuv_buf_8b[y * img_w * 2 + x] = src_img[y * img_w * 2 + x] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
        else if(m_yuvType == YuvType::YUV444_INTERLEAVE)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 3]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w * 3; x += 3)
                {
                    uint16_t gray = src_img[y * img_w * 3 + x];
                    uint16_t idx0 = gray >> (m_srcImgBits - 8);
                    uint16_t idx1 = qMin(idx0 + 1, 256);
                    uint16_t delta = gray - (idx0 << (m_srcImgBits - 8));
                    yuv_buf_8b[y * img_w * 3 + x] = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));

                    yuv_buf_8b[y * img_w * 3 + x + 1] = src_img[y * img_w * 3 + x + 1] >> (m_srcImgBits - 8);
                    yuv_buf_8b[y * img_w * 3 + x + 2] = src_img[y * img_w * 3 + x + 2] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
        else if(m_yuvType == YuvType::YUV444_PLANAR)
        {
            int entirpixperline = m_dispImage->bytesPerLine() / 3;
            unsigned char *bufferShow = m_dispImage->bits();

            std::unique_ptr<uint8_t[]> yuv_buf_8b(new uint8_t[img_h * img_w * 3]);

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w; x += 1)
                {
                    uint16_t gray = src_img[y * img_w + x];
                    uint16_t idx0 = gray >> (m_srcImgBits - 8);
                    uint16_t idx1 = qMin(idx0 + 1, 256);
                    uint16_t delta = gray - (idx0 << (m_srcImgBits - 8));
                    yuv_buf_8b[y * img_w + x] = static_cast<uint8_t>(((uint32_t)srcToDispLut[idx0] * (step - delta) + (uint32_t)srcToDispLut[idx1] * delta) >> (m_srcImgBits - 8));
                }
            }

            for (int y = 0; y < img_h; y++)
            {
                for (int x = 0; x < img_w * 2; x++)
                {
                    yuv_buf_8b[img_h * img_w + y * img_w * 2 + x] = src_img[img_h * img_w + y * img_w * 2 + x] >> (m_srcImgBits - 8);
                }
            }

            convertYUV2RGB888(yuv_buf_8b.get(), bufferShow, 8, img_w, img_h, m_yuvType, entirpixperline);
        }
    }
}

void CurveAdjustDialog::onResetClicked()
{
    // qint32 max_v = (1 << m_srcImgBits) - 1;
    // qint32 v0 = (max_v + 1) / 8;
    // qint32 v1 = (max_v + 1) / 4;
    // qint32 v2 = (max_v + 1) * 3 / 8;
    // qint32 v3 = (max_v + 1) / 2;
    // qint32 v4 = (max_v + 1) * 5 / 8;
    // qint32 v5 = (max_v + 1) * 3 / 4;
    // qint32 v6 = (max_v + 1) * 7 / 8;

    // std::array<QPoint, 9> reset_curve = {QPoint{0, 0}, QPoint{v0, 32}, QPoint{v1, 64}, QPoint{v2, 96}, QPoint{v3, 128}, QPoint{v4, 160}, QPoint{v5, 192}, QPoint{v6, 224}, {max_v, 255}};

    m_rgbCurve[0].setY(0);
    m_rgbCurve[1].setY(32);
    m_rgbCurve[2].setY(64);
    m_rgbCurve[3].setY(96);
    m_rgbCurve[4].setY(128);
    m_rgbCurve[5].setY(160);
    m_rgbCurve[6].setY(192);
    m_rgbCurve[7].setY(224);
    m_rgbCurve[8].setY(255);

    updateChartFromCurve();
    applyCurveToImage();
    emit updateDisp();
}

void CurveAdjustDialog::onApplyClicked()
{
    // accept();
    updateCurveFromChart();
    applyCurveToImage();
    emit updateDisp();
}

void CurveAdjustDialog::onCancelClicked()
{
    reject();
}

void CurveAdjustDialog::calculateSplineLutFromCurve()
{
    // Convert QPoint array to QPointF vector for spline calculation
    QVector<QPointF> points;
    for (const auto &point : m_rgbCurve)
    {
        points.append(QPointF(point.x(), point.y()));
    }

    // Ensure points are sorted by x coordinate
    // std::sort(points.begin(), points.end(), 
    //           [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); });

    const int n = points.size();
    if (n < 2)
    {
        // Not enough points for spline, use linear mapping
        uint32_t step = (1 << m_srcImgBits) / 256;
        for (int i = 0; i < 256; i++)
        {
            srcToDispLut[i] = i * step;
        }
        if (m_srcImgBits > 8)
        {
            srcToDispLut[255] -= 1;
        }
        return;
    }

    // Calculate natural cubic spline coefficients
    QVector<double> h(n - 1);  // differences between x values
    QVector<double> alpha(n);  // intermediate values
    QVector<double> l(n);      // diagonal elements
    QVector<double> mu(n);     // off-diagonal elements
    QVector<double> z(n);      // right-hand side
    QVector<double> c(n);      // second derivatives

    // Step 1: Calculate h[i] = x[i+1] - x[i]
    for (int i = 0; i < n - 1; i++)
    {
        h[i] = points[i + 1].x() - points[i].x();
    }

    // Step 2: Calculate alpha[i]
    for (int i = 1; i < n - 1; i++)
    {
        alpha[i] = (3.0 / h[i]) * (points[i + 1].y() - points[i].y()) -
                   (3.0 / h[i - 1]) * (points[i].y() - points[i - 1].y());
    }

    // Step 3: Solve tridiagonal system
    l[0] = 1.0;
    mu[0] = 0.0;
    z[0] = 0.0;

    for (int i = 1; i < n - 1; i++)
    {
        l[i] = 2.0 * (points[i + 1].x() - points[i - 1].x()) - h[i - 1] * mu[i - 1];
        mu[i] = h[i] / l[i];
        z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
    }

    l[n - 1] = 1.0;
    z[n - 1] = 0.0;
    c[n - 1] = 0.0;
    
    // Back substitution
    for (int i = n - 2; i >= 0; i--)
    {
        c[i] = z[i] - mu[i] * c[i + 1];
    }

    // Calculate b and d coefficients
    QVector<double> b(n - 1);
    QVector<double> d(n - 1);

    for (int i = 0; i < n - 1; i++)
    {
        b[i] = (points[i + 1].y() - points[i].y()) / h[i] -
               h[i] * (c[i + 1] + 2.0 * c[i]) / 3.0;
        d[i] = (c[i + 1] - c[i]) / (3.0 * h[i]);
    }

    // Populate srcToDispLut using spline interpolation
    uint32_t max_input = (1 << m_srcImgBits) - 1;
    
    for (int i = 0; i < 257; i++)
    {
        double x = i < 256 ? static_cast<double>(i) * (max_input + 1) / 256.0 : max_input;
        
        // Find the segment that contains x
        int segment = 0;
        while (segment < n - 1 && x > points[segment + 1].x())
        {
            segment++;
        }
        
        if (segment >= n - 1)
        {
            // x is beyond the last point, use last point's y value
            srcToDispLut[i] = static_cast<uint32_t>(points[n - 1].y());
            continue;
        }
        
        if (x < points[0].x())
        {
            // x is before the first point, use first point's y value
            srcToDispLut[i] = static_cast<uint32_t>(points[0].y());
            continue;
        }
        
        // Evaluate cubic spline: S(x) = a + b*(x-x_i) + c*(x-x_i)^2 + d*(x-x_i)^3
        double dx = x - points[segment].x();
        double y = points[segment].y() + 
                   b[segment] * dx + 
                   c[segment] * dx * dx + 
                   d[segment] * dx * dx * dx;
        
        // Clamp to valid range [0, 255]
        y = qBound(0.0, y, 255.0);
        srcToDispLut[i] = static_cast<uint32_t>(y);
    }

    // for(int i=0; i<257; i++)
    //     qDebug() << srcToDispLut[i];
}
