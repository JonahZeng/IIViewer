#ifndef IIVIEWER_H
#define IIVIEWER_H

#include "AppSetting.h"
#include "IIViewer_ui.h"
#include <QtWidgets/QMainWindow>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QPushButton>

class QScreen;
class QWindow;

class IIViewer final: public QMainWindow
{
    Q_OBJECT

public:
    explicit IIViewer(QString& needOpenFilePath, QWidget *parent = nullptr);
    virtual ~IIViewer();
    void setTitle();
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void changeEvent(QEvent *event) override;
#ifdef Q_OS_WINDOWS
    #if QT_VERSION_MAJOR < 6
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    #else
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    #endif
#endif

private:
    void applyDpiScale(QScreen *screen);
    void ensureWindowHandleConnections();
    void loadFile(const QString &fileName, int scrollArea);
    void reLoadFile(int scrollArea);
    void loadYuvFile(const QString &fileName, int scrollArea, bool reload=false);
    void loadRawFile(const QString &fileName, int scrollArea, bool reload=false);
    void loadPnmFile(const QString &fileName, int scrollArea, bool reload=false);
    void loadPgmFile(const QString &fileName, int scrollArea, bool reload=false);
    void loadHeifFile(const QString &fileName, int scrollArea, bool reload=false);
    void loadDngFile(const QString &fileName, int scrollArea, bool reload=false);
    void loadFilePostProcessLayoutAndScrollValue(int leftOrRight);
    void setImage(QString &image, int leftOrRight);
    void setYuvImage(QString &imageName, YuvType tp, int bitDepth, int width, int height, int pixSize, int leftOrRight);
    void setRawImage(QString &image, BayerPatternType bay, ByteOrderType order, int bitDepth, bool compact, int width, int height, int leftOrRight);
    void setDngRawImage(QString &image, BayerPatternType bay, ByteOrderType order, int bitDepth, bool compact, int width, int height, int leftOrRight, const uint8_t* raw_buffer);
    void setDngPnmImage(QString &imageName, int bitDepth, int width, int height, int leftOrRight, const uint8_t* rgb_buffer);
    void openGivenFileFromCmdArgv(QString& image);
    void addFileToHistory(const QString &filePath);
    void scrollToPixelPos(const QPoint& pixelPos);

protected:
    void showEvent(QShowEvent *event) override;

public slots:
    void onOpenFileAction();
    void onCloseLeftFileAction();
    void onCloseRightFileAction();
    void onReloadFileAction();
    void setPenWidth(int width);
    void setTheme();
    void onUseRoiAction(bool check);
    void onUseMoveAction(bool check);
    void onSingleImgModeAction(bool check);
    void onDoubleImgModeAction(bool check);
    void onSysOptionAction(bool check);
    void toggleDataAnalyseDockWgt(bool checked);
    void togglePlayListDockWgt(bool checked);
    void syncRightPos() const;
    void syncLeftPos() const;
    void flushPaintPosEdit0(QPointF startPt, QPointF endPt) const;
    void flushPaintPosEdit1(QPointF startPt, QPointF endPt) const;
    void handleInputPaintPos0();
    void handleInputPaintPos1();
    void plotContourf();
    // void plotRgbHist();
    void selectPenPaintColor();
    void zoomIn0(int zoomIdx);
    void zoomIn1(int zoomIdx);
    void zoomOut0(int zoomIdx);
    void zoomOut1(int zoomIdx);
    void clearPaint();
    void handleRightMouseBtnDrag0(QPointF startPt, QPointF endPt);
    void handleRightMouseBtnDrag1(QPointF startPt, QPointF endPt);
    void setScrollArea1Master()
    {
        masterScrollarea = ui.scrollArea[1];
    }
    void setScrollArea0Master()
    {
        masterScrollarea = ui.scrollArea[0];
    }
    void syncScrollArea1_horScBarVal(int);
    void syncScrollArea1_verScBarVal(int);
    void syncScrollArea0_horScBarVal(int);
    void syncScrollArea0_verScBarVal(int);
    void updateExchangeBtn();
    void updateZoomLabelText();
    void exchangeRight2LeftImg();
    void restoreLeftImg();
    void showImageInfo();
    void showImageDiffReport();
    void checkUpdate();
    void openedFileChanged(const QString& filePath);

signals:
    void updateExchangeBtnStatus();
    void updateZoomLabelStatus();
    void needOpenFileFromCmdArgv(QString& image);

public:
    Ui::IIViewerUi ui;
    QString workPath;
    std::array<QSize, 2> originSize;
    std::array<QString, 2> openedFile;
    std::array<QDateTime, 2> openedFileLastModifiedTime;
    QFileSystemWatcher openedFileWatcher;
    std::array<QDateTime, 2> lastFileWatcherNotifyTime; // on windows, rewrite file will notify 2 same message, record first messge path and time, ignore second message
    std::array<QString, 2> lastFileWatcherNotifyPath;
    std::array<bool, 2> lastFileWatcherNotifyIsWaitProcess;
    QScrollArea *masterScrollarea;
    AppSettings settings;
    bool windowHandleConnectionsInitialized = false;
    QPushButton *ncHoveredButton = nullptr;
};

constexpr int LEFT_IMG_WIDGET = 0;
constexpr int RIGHT_IMG_WIDGET = 1;
constexpr int BIT8 = 8;
constexpr int BIT16 = 16;
// constexpr int BIT24 = 24;
constexpr int BIT32 = 32;

#endif // IIViewer_H
