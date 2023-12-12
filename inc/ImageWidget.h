#pragma once
#include "RawFileInfoDlg.h"
#include "YuvFileInfoDlg.h"
#include <QColor>
#include <QEvent>
#include <QImage>
#include <QScrollArea>
#include <QScrollBar>
#include <QWidget>
#include <array>

enum OpenedImageType {
    NORMAL_IMG = 1,
    RAW_IMG = 2,
    PNM_IMG = 3,
    YUV_IMG = 4,
    UNKNOW_IMG = -1
};

enum MouseActionType {
    NONE_ACTION = -1,
    PAINT_ROI_ACTION = 0,
    DRAG_IMG_ACTION = 1
};

#define ZOOM_LIST_LENGTH 13

class ImageWidget : public QWidget {
    Q_OBJECT
public:
    ImageWidget() = delete;
    ImageWidget(QColor color, int penWidth, QScrollArea* parentScroll, QWidget* parent = nullptr);
    ~ImageWidget();
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void setPixmap(QString& img);
    void setPixmap(QString& img, RawFileInfoDlg::BayerPatternType by, RawFileInfoDlg::ByteOrderType order, int bitDepth, int width, int height);
    void setPixmap(QString& img, YuvFileInfoDlg::YuvType tp, int bitDepth, int width, int height, int pixSize);
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
    void acceptImgFromOther(const ImageWidget* other);
    void restoreImg();

private:
    void processWheelZoom(QWheelEvent* event, int zoomDelta);
    void paintBitMapPixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    void paintRawPixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    void paintPnmPixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    void paintYuv444InterleavePixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    void paintYuv444PlanarPixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    void paintYuv422UYVYPixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    void paintYuv422YUYVPixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    void paintYuv420NV12PixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    void paintYuv420NV21PixVal(QPoint& viewTopLeftPix, QPainter& painter, int viewPixWidth, int viewPixHeight, QPoint& paintPixValTopLeft);
    RawFileInfoDlg::BayerPixelType getPixType(int y, int x, RawFileInfoDlg::BayerPatternType by);

public:
    QScrollArea* parentScroll;
    MouseActionType mouseAction;
    QColor penColor;
    int penWidth;
    std::array<QPoint, 2> paintCoordinates;
    bool paintBegin;
    bool paintEnd;
    bool doDragImg;
    QPoint imgDragStartPos;
    QPoint imgDragEndPos;
    QImage* pixMap;
    int zoomIdx;
    const std::array<float, ZOOM_LIST_LENGTH> zoomList;
    QRectF zoomTextRect;
    QRectF pixValPaintRect;
    unsigned char* rawDataPtr;
    int rawDataBit;
    unsigned char* pnmDataPtr;
    int pnmDataBit;
    unsigned char* yuvDataPtr;
    int yuvDataBit;
    RawFileInfoDlg::BayerPatternType rawBayerType;
    RawFileInfoDlg::ByteOrderType rawByteOrderType;
    YuvFileInfoDlg::YuvType yuvType;
    OpenedImageType openedImgType;

private:
    QImage* pixMapBak;
    unsigned char* rawDataPtrBak;
    int rawDataBitBak;
    unsigned char* pnmDataPtrBak;
    int pnmDataBitBak;
    unsigned char* yuvDataPtrBak;
    int yuvDataBitBak;
    RawFileInfoDlg::BayerPatternType rawBayerTypeBak;
    RawFileInfoDlg::ByteOrderType rawByteOrderTypeBak;
    YuvFileInfoDlg::YuvType yuvTypeBak;
    OpenedImageType openedImgTypeBak;
signals:
    void inform_real_Pos(QPointF, QPointF);
    void inform_drag_img(QPoint, QPoint);
    void inform_zoom_in(int);
    void inform_zoom_out(int);
};
