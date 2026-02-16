#pragma once

#include "common_type.h"
#include <QDialog>
#include <QVector>
#include <QPointF>
#include <QImage>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QScatterSeries>
#include <array>


class QComboBox;
class QPushButton;
class QLabel;

QT_CHARTS_USE_NAMESPACE

class CurveAdjustChart final : public QChart
{
    Q_OBJECT
public:
    CurveAdjustChart() = delete;
    CurveAdjustChart(quint16 m_srcImgBits);
    ~CurveAdjustChart() = default;
private slots:
    void pointSelected(QPointF pst);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
public:
    QSplineSeries* m_currentSpSeries;
    QScatterSeries* m_currentScaSeries;
private:
    void initUI();
    int m_selectedPointIdx;
    bool m_dragging;
    quint16 m_srcImgBits;
};

class CurveAdjustDialog final : public QDialog
{
    Q_OBJECT

public:
    CurveAdjustDialog() = delete;
    CurveAdjustDialog(QImage* disp_image, void* src_img, OpenedImageType img_type, quint16 src_img_bits, const std::array<QPoint, 9>& preset_curve, QWidget* parent = nullptr);
    ~CurveAdjustDialog() = default;
    void setYuvType(YuvType tp)
    {
        if(m_originImgType == OpenedImageType::YUV_IMG)
        {
            m_yuvType = tp;
        }
    }
signals:
    void updateDisp(const std::array<QPoint, 9>&);

private slots:
    void onResetClicked();
    void onApplyClicked();
    void onCancelClicked();

private:
    void initUI();
    void applyCurveToImage();
    void updateChartFromCurve();
    void updateCurveFromChart();
    void calculateSplineLutFromCurve();

private:
    void* m_originalImage;
    OpenedImageType m_originImgType;
    QImage* m_dispImage;
    YuvType m_yuvType;
    
    QPushButton* m_resetButton;
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
    QChartView* m_chartView;
    CurveAdjustChart* m_chart;

    std::array<QPoint, 9> m_rgbCurve;

    quint16 m_srcImgBits;
    std::array<uint32_t, 257> srcToDispLut; // 映射曲线，X轴采样256点[0, 256]
};
