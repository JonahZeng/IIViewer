#ifndef IIPVIEWER_H
#define IIPVIEWER_H

#include "AppSetting.h"
#include "IIPviewer_ui.h"
#include <QtWidgets/QMainWindow>
#include <QFileSystemWatcher>
#include <QDateTime>

class IIPviewer final: public QMainWindow
{
    Q_OBJECT

public:
    IIPviewer(QString needOpenFilePath, QWidget *parent = nullptr);
    virtual ~IIPviewer();
    void setTitle();
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void loadFile(QString &fileName, int scrollArea);
    void reLoadFile(int scrollArea);
    void loadYuvFile(QString &fileName, int scrollArea, bool reload=false);
    void loadRawFile(QString &fileName, int scrollArea, bool reload=false);
    void loadPnmFile(QString &fileName, int scrollArea, bool reload=false);
    void loadPgmFile(QString &fileName, int scrollArea, bool reload=false);
    void loadFilePostProcessLayoutAndScrollValue(int leftOrRight);
    void setImage(QString &image, int leftOrRight);
    void setYuvImage(QString &imageName, YuvFileInfoDlg::YuvType tp, int bitDepth, int width, int height, int pixSize, int leftOrRight);
    void setRawImage(QString &image, RawFileInfoDlg::BayerPatternType by, RawFileInfoDlg::ByteOrderType order, int bitDepth, bool compact, int width, int height, int leftOrRight);
    void openGivenFileFromCmdArgv(QString image);

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
    void toggleDataAnalyseDockWgt(bool checked);
    void togglePlayListDockWgt(bool checked);
    void syncRightPos();
    void syncLeftPos();
    void flushPaintPosEdit0(QPointF startPt, QPointF endPt);
    void flushPaintPosEdit1(QPointF startPt, QPointF endPt);
    void handleInputPaintPos0();
    void handleInputPaintPos1();
    void plotRgbContourf();
    void plotRgbHist();
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
    void checkUpdate();
    void openedFileChanged(const QString& filePath);

signals:
    void updateExchangeBtnStatus();
    void updateZoomLabelStatus();
    void needOpenFileFromCmdArgv(QString image);

public:
    Ui::IIPviewerUi ui;
    QColor penColor;
    QString workPath;
    std::array<QSize, 2> originSize;
    std::array<QString, 2> openedFile;
    std::array<QDateTime, 2> openedFileLastModifiedTime;
    QFileSystemWatcher openedFileWatcher;
    std::array<QDateTime, 2> lastFileWatcherNotifyTime; // on windows, rewrite file will notify 2 same message, record first messge path and time, ignore second message
    std::array<QString, 2> lastFileWatcherNotifyPath;
    QScrollArea *masterScrollarea;
    AppSettings settings;
};

#endif // IIPVIEWER_H
