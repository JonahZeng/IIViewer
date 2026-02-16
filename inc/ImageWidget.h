#pragma once
#include "RawFileInfoDlg.h"
#include "YuvFileInfoDlg.h"
#include "common_type.h"
#include "IIPOptionDialog.h"
#include "AppSetting.h"
#include <QColor>
#include <QEvent>
#include <QImage>
#include <QScrollArea>
#include <QScrollBar>
#include <QWidget>
#include <QMenu>
#include <array>


struct PaintCoordinateInfo {
    std::array<QPoint, 2> paintCoordinates;       // 记录鼠标绘制矩后的坐标，跟随倍率变换
    std::array<QPoint, 2> originPaintCoordinates; // 记录鼠标绘制矩形时的原始坐标
    float originScaleRatio;                       // 记录鼠标绘制矩形时的原始倍率
};

#define ZOOM_LIST_LENGTH 13
constexpr int IMG_PREVIEW_ADJ_CURVE_DOTS = 9;

class ImageWidget final : public QWidget
{
    Q_OBJECT
public:
    ImageWidget() = delete;
    ImageWidget(QColor color, int penWidth, QScrollArea *parentScroll, QWidget *parent = nullptr);
    ~ImageWidget();
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void setPixmap();
    void setPixmap(BayerPatternType by, ByteOrderType order, int bitDepth, bool compact, int width, int height);
    void setPixmap(YuvType tp, int bitDepth, int width, int height, int pixSize);
    void zoomIn(int zoomIdx);
    void zoomOut(int zoomIdx);
    void setMouseActionPaintRoi()
    {
        mouseAction = MouseActionType::PAINT_ROI_ACTION;
    }

    void setMouseActionDragImg()
    {
        mouseAction = MouseActionType::DRAG_IMG_ACTION;
    }

    void setMouseActionNone()
    {
        mouseAction = MouseActionType::NONE_ACTION;
    }

    void setPenWidth(int width)
    {
        penWidth = width;
        repaint();
    }

    void setPaintPenColor(QColor color)
    {
        penColor = color;
        repaint();
    }
    void releaseBuffer();
    void acceptImgFromOther(const ImageWidget *other);
    void restoreImg();

private:
    void processWheelZoom(QWheelEvent *event, int zoomDelta);
    void paintBitMapPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintRawPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintPnmPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintPgmPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv444InterleavePixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv444PlanarPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv422UYVYPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv422YUYVPixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv420NV12PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv420NV21PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv420PYU12PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv420PYV12PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void paintYuv400PixVal(QPoint &viewTopLeftPix, QPainter &painter, int viewPixWidth, int viewPixHeight, QPoint &paintPixValTopLeft);
    void setPainterColorShape(QPainter &painter);
    BayerPixelType getPixType(int y, int x, BayerPatternType by);
    QString generateRoiDataStr();
    void showRoiDataToText();
    void exportRoiDataToDisk();
    void exportRoiYuvData(QString &roiPixelValStr, int roi_top, int roi_bottom, int roi_left, int roi_right);
    void adjustPreviewCurve();

public:
    QMenu rightMouseContextMenu;
    QScrollArea *parentScroll;
    MouseActionType mouseAction; // 标记鼠标作用
    QColor penColor;
    int penWidth;
    PaintCoordinateInfo ptCodInfo;
    bool paintBegin;
    bool paintEnd;
    bool doDragImg;
    QPoint imgDragStartPos;
    QPoint imgDragEndPos;
    QImage *pixMap;
    int zoomIdx;
    const std::array<float, ZOOM_LIST_LENGTH> zoomList;
    const std::array<QString, ZOOM_LIST_LENGTH> zoomListLabel;
    QRectF zoomTextRect;
    QRectF pixValPaintRect;
    unsigned char *rawDataPtr;
    int rawDataBit;
    unsigned char *pnmDataPtr;
    int pnmDataBit;
    unsigned char *pgmDataPtr;
    int pgmDataBit;
    unsigned char *yuvDataPtr;
    int yuvDataBit;
    QImage *normalDataPixMap;
    BayerPatternType rawBayerType;
    ByteOrderType rawByteOrderType;
    YuvType yuvType;
    OpenedImageType openedImgType;
    AppSettings* appSettings; // app设置信息，会通过顶层IIPviewer(QMainWindow)传递过来，只使用不持有
    QString* imgName; // 保存IIPviewer::openedFile
    std::array<QPoint, IMG_PREVIEW_ADJ_CURVE_DOTS> lastCurve;

private:
    QImage *pixMapBak;
    unsigned char *rawDataPtrBak;
    int rawDataBitBak;
    unsigned char *pnmDataPtrBak;
    int pnmDataBitBak;
    unsigned char *pgmDataPtrBak;
    int pgmDataBitBak;
    unsigned char *yuvDataPtrBak;
    int yuvDataBitBak;
    QImage *normalDataPixMapBak;
    BayerPatternType rawBayerTypeBak;
    ByteOrderType rawByteOrderTypeBak;
    YuvType yuvTypeBak;
    OpenedImageType openedImgTypeBak;
signals:
    void inform_real_Pos(QPointF, QPointF);
    void inform_drag_img(QPoint, QPoint);
    void inform_zoom_in(int);
    void inform_zoom_out(int);
    void inform_change_master();
    void inform_open_file_selector();
};
