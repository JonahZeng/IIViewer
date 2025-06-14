#include "config.h"
#include "IIPviewer.h"
#include "AboutDlg.h"
#include "DataVisualDlg.h"
#include "IIPOptionDialog.h"
#include <QApplication>
#include <QColorDialog>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeData>
#include <QScreen>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QStyleFactory>

constexpr int LEFT_IMG_WIDGET = 0;
constexpr int RIGHT_IMG_WIDGET = 1;

class ImgInfoDlg : public QDialog
{
    // Q_OBJECT
public:
    ImgInfoDlg(QWidget *parent)
        : QDialog(parent)
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
    ~ImgInfoDlg()
    {
    }

    void setImgInfo(QString &filePath, QSize &imgSize, RawFileInfoDlg::BayerPatternType by, YuvFileInfoDlg::YuvType yt, int bits, bool left)
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
        if (by == RawFileInfoDlg::BayerPatternType::RGGB)
        {
            tmp += QString(" RGGB");
        }
        else if (by == RawFileInfoDlg::BayerPatternType::GRBG)
        {
            tmp += QString(" GRBG");
        }
        else if (by == RawFileInfoDlg::BayerPatternType::GBRG)
        {
            tmp += QString(" GBRG");
        }
        else if (by == RawFileInfoDlg::BGGR)
        {
            tmp += QString(" BGGR");
        }
        if (yt == YuvFileInfoDlg::YuvType::YUV444_INTERLEAVE)
        {
            tmp += QString(" YUV444_INTERLEAVE");
        }
        else if (yt == YuvFileInfoDlg::YuvType::YUV444_PLANAR)
        {
            tmp += QString(" YUV444_PLANAR");
        }
        else if (yt == YuvFileInfoDlg::YuvType::YUV422_UYVY)
        {
            tmp += QString(" YUV422_UYVY");
        }
        else if (yt == YuvFileInfoDlg::YuvType::YUV422_YUYV)
        {
            tmp += QString(" YUV422_YUYV");
        }
        else if (yt == YuvFileInfoDlg::YuvType::YUV420_NV12)
        {
            tmp += QString(" YUV420_NV12");
        }
        else if (yt == YuvFileInfoDlg::YuvType::YUV420_NV21)
        {
            tmp += QString(" YUV420_NV21");
        }
        else if (yt == YuvFileInfoDlg::YuvType::YUV420P_YU12)
        {
            tmp += QString(" YUV420P_YU12");
        }
        else if (yt == YuvFileInfoDlg::YuvType::YUV420P_YV12)
        {
            tmp += QString(" YUV420P_YV12");
        }
        else if (yt == YuvFileInfoDlg::YuvType::YUV400)
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

private:
    QLabel *leftLabel;
    QLabel *rightLabel;
};

IIPviewer::IIPviewer(QString needOpenFilePath, QWidget *parent)
    : QMainWindow(parent), ui(), 
    penColor(0, 0, 0), 
    originSize{QSize{0, 0}, QSize{0, 0}}, 
    openedFile{QString(), QString()},
    openedFileLastModifiedTime{QDateTime(), QDateTime()},
    openedFileWatcher{},
    lastFileWatcherNotifyTime{QDateTime(), QDateTime()},
    lastFileWatcherNotifyPath{QString(), QString()},
    lastFileWatcherNotifyIsWaitProcess{false, false}
{
    if (settings.loadSettingsFromFile())
    {
        workPath = settings.workPath;
    }
    else
    {
        auto pathList = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        if (pathList.length() > 0)
        {
            workPath = pathList.last();
        }
        else
        {
            workPath = QDir::currentPath();
        }
    }
    int theme_idx = 0;
    if(QStyleFactory::keys().contains(settings.theme))
    {    
        QApplication::setStyle(settings.theme);
        theme_idx = QStyleFactory::keys().indexOf(settings.theme);
    }

    setAcceptDrops(true);
    ui.setupUi(this);
    ui.imageLabel[LEFT_IMG_WIDGET]->appSettings = &settings;
    ui.imageLabel[RIGHT_IMG_WIDGET]->appSettings = &settings;

    QList<QScreen *> screenInfoList = QApplication::screens();
    bool prevScreenExist = false;
    for(auto sc: screenInfoList)
    {
        if(sc->name() == settings.windowScreenName)
        {
            prevScreenExist = true;
            break;
        }
    }
    if(!prevScreenExist)
    {
        QRect screenRect = screenInfoList.at(0)->geometry();
        setGeometry(screenRect.width() / 6, screenRect.height() / 6, screenRect.width() * 2 / 3, screenRect.height() * 2 / 3);
    }
    else
    {
        // Restore window geometry from saved settings
        QRect geometry = settings.windowGeometry;
        
        // Ensure window is visible on current screen
        QScreen *screen = QApplication::screenAt(geometry.center());
        if (screen) {
            QRect screenRect = screen->geometry();
            geometry = geometry.intersected(screenRect);
        }
        
        setGeometry(geometry);
    }
    setTitle();

    masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];
    // set theme idx by settings
    for(int t_idx=0; t_idx < ui.themes.size(); t_idx++)
    {
        ui.themes[t_idx]->setChecked(t_idx == theme_idx ? true : false);
    }

    connect(ui.openFileLeftAction, &QAction::triggered, this, &IIPviewer::onOpenFileAction);
    connect(ui.openFileRightAction, &QAction::triggered, this, &IIPviewer::onOpenFileAction);
    connect(ui.reloadFileLeftAction, &QAction::triggered, this, &IIPviewer::onReloadFileAction);
    connect(ui.reloadFileRightAction, &QAction::triggered, this, &IIPviewer::onReloadFileAction);
    connect(ui.exchangeAreaPreviewBtn, &QPushButton::pressed, this, &IIPviewer::exchangeRight2LeftImg);
    connect(ui.exchangeAreaPreviewBtn, &QPushButton::released, this, &IIPviewer::restoreLeftImg);
    connect(ui.imageInfoBtn, &QPushButton::clicked, this, &IIPviewer::showImageInfo);
    connect(ui.closeLeftAction, &QAction::triggered, this, &IIPviewer::onCloseLeftFileAction);
    connect(ui.closeRightAction, &QAction::triggered, this, &IIPviewer::onCloseRightFileAction);
    connect(ui.exitAction, &QAction::triggered, this, [this]()
            { this->close(); });
    // connect(ui.aboutQtAction, &QAction::triggered, []()
    //         { qApp->aboutQt(); });
    connect(ui.aboutThisAction, &QAction::triggered, this, [this]()
            { AboutDlg dlg(this); dlg.exec(); });
    connect(ui.checkUpdateAction, &QAction::triggered, this, &IIPviewer::checkUpdate);

    connect(ui.penColorSetBtn, &QPushButton::clicked, this, &IIPviewer::selectPenPaintColor);
    connect(ui.penWidthSbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &IIPviewer::setPenWidth);
    for (auto &theme : ui.themes)
    {
        connect(theme, &QAction::triggered, this, &IIPviewer::setTheme);
    }

    connect(ui.useMoveToolAction, &QAction::triggered, this, &IIPviewer::onUseMoveAction);
    connect(ui.useRoiToolAction, &QAction::triggered, this, &IIPviewer::onUseRoiAction);
    connect(ui.sysOptionAction, &QAction::triggered, this, &IIPviewer::onSysOptionAction);
    connect(ui.workAreaSingleModeAction, &QAction::triggered, this, &IIPviewer::onSingleImgModeAction);
    connect(ui.workAreaDoubleModeAction, &QAction::triggered, this, &IIPviewer::onDoubleImgModeAction);

    connect(ui.paintOk0, &QPushButton::clicked, this, &IIPviewer::handleInputPaintPos0);
    connect(ui.paintOk1, &QPushButton::clicked, this, &IIPviewer::handleInputPaintPos1);
    connect(ui.syncRight, &QPushButton::clicked, this, &IIPviewer::syncRightPos);
    connect(ui.syncLeft, &QPushButton::clicked, this, &IIPviewer::syncLeftPos);
    connect(ui.clearPaintBtn, &QPushButton::clicked, this, &IIPviewer::clearPaint);

    connect(ui.plot_rgb_contourf_line, &QPushButton::clicked, this, &IIPviewer::plotRgbContourf);
    connect(ui.plot_rgb_hist, &QPushButton::clicked, this, &IIPviewer::plotRgbHist);
    connect(ui.imageLabel[LEFT_IMG_WIDGET], &ImageWidget::inform_real_Pos, this, &IIPviewer::flushPaintPosEdit0);
    connect(ui.imageLabel[RIGHT_IMG_WIDGET], &ImageWidget::inform_real_Pos, this, &IIPviewer::flushPaintPosEdit1);
    connect(ui.imageLabel[LEFT_IMG_WIDGET], &ImageWidget::inform_drag_img, this, &IIPviewer::handleRightMouseBtnDrag0);
    connect(ui.imageLabel[RIGHT_IMG_WIDGET], &ImageWidget::inform_drag_img, this, &IIPviewer::handleRightMouseBtnDrag1);
    connect(ui.imageLabel[LEFT_IMG_WIDGET], &ImageWidget::inform_zoom_in, this, &IIPviewer::zoomIn1);
    connect(ui.imageLabel[LEFT_IMG_WIDGET], &ImageWidget::inform_zoom_out, this, &IIPviewer::zoomOut1);
    connect(ui.imageLabel[RIGHT_IMG_WIDGET], &ImageWidget::inform_zoom_in, this, &IIPviewer::zoomIn0);
    connect(ui.imageLabel[RIGHT_IMG_WIDGET], &ImageWidget::inform_zoom_out, this, &IIPviewer::zoomOut0);
    connect(ui.imageLabel[LEFT_IMG_WIDGET], &ImageWidget::inform_change_master, this, [this]()
        {
            this->masterScrollarea = this->ui.scrollArea[LEFT_IMG_WIDGET];
        });
    connect(ui.imageLabel[RIGHT_IMG_WIDGET], &ImageWidget::inform_change_master, this, [this]()
        {
            this->masterScrollarea = this->ui.scrollArea[RIGHT_IMG_WIDGET];
        });

    connect(ui.scrollArea[LEFT_IMG_WIDGET]->horizontalScrollBar(), &QScrollBar::valueChanged, this, &IIPviewer::syncScrollArea1_horScBarVal);
    connect(ui.scrollArea[LEFT_IMG_WIDGET]->verticalScrollBar(), &QScrollBar::valueChanged, this, &IIPviewer::syncScrollArea1_verScBarVal);
    connect(ui.scrollArea[LEFT_IMG_WIDGET]->horizontalScrollBar(), &QScrollBar::sliderPressed, this, &IIPviewer::setScrollArea0Master);
    connect(ui.scrollArea[LEFT_IMG_WIDGET]->verticalScrollBar(), &QScrollBar::sliderPressed, this, &IIPviewer::setScrollArea0Master);
    connect(ui.scrollArea[RIGHT_IMG_WIDGET]->horizontalScrollBar(), &QScrollBar::valueChanged, this, &IIPviewer::syncScrollArea0_horScBarVal);
    connect(ui.scrollArea[RIGHT_IMG_WIDGET]->horizontalScrollBar(), &QScrollBar::valueChanged, this, &IIPviewer::syncScrollArea0_horScBarVal);
    connect(ui.scrollArea[RIGHT_IMG_WIDGET]->verticalScrollBar(), &QScrollBar::valueChanged, this, &IIPviewer::syncScrollArea0_verScBarVal);
    connect(ui.scrollArea[RIGHT_IMG_WIDGET]->horizontalScrollBar(), &QScrollBar::sliderPressed, this, &IIPviewer::setScrollArea1Master);
    connect(ui.scrollArea[RIGHT_IMG_WIDGET]->verticalScrollBar(), &QScrollBar::sliderPressed, this, &IIPviewer::setScrollArea1Master);

    connect(ui.dataAnalyseAction, &QAction::toggled, this, &IIPviewer::toggleDataAnalyseDockWgt);
    connect(ui.playListAction, &QAction::toggled, this, &IIPviewer::togglePlayListDockWgt);

    connect(this, &IIPviewer::updateExchangeBtnStatus, this, &IIPviewer::updateExchangeBtn);
    connect(this, &IIPviewer::updateZoomLabelStatus, this, &IIPviewer::updateZoomLabelText);
    connect(this, &IIPviewer::needOpenFileFromCmdArgv, this, &IIPviewer::openGivenFileFromCmdArgv);

    connect(&openedFileWatcher, &QFileSystemWatcher::fileChanged, this, &IIPviewer::openedFileChanged);

    ui.dataAnalyseDockWgt->installEventFilter(this);
    ui.playListDockWgt->installEventFilter(this);
    ui.mainWidget->installEventFilter(this);

    ui.dataAnalyseDockWgt->hide();
    ui.playListDockWgt->hide();
    ui.dataAnalyseAction->setChecked(false);
    ui.playListAction->setChecked(false);
    ui.exchangeAreaPreviewBtn->setEnabled(false);
    setWindowIcon(QIcon(":image/resource/aboutlog.png"));

    if(settings.workAreaDoubleImgMode)
    {
        ui.workAreaDoubleModeAction->setChecked(true);
        onDoubleImgModeAction(true);
    }
    else
    {
        ui.workAreaSingleModeAction->setChecked(true);
        onSingleImgModeAction(true);
    }

    if(needOpenFilePath.length() > 0)
    {
        emit needOpenFileFromCmdArgv(needOpenFilePath);
    }
}

void IIPviewer::showEvent(QShowEvent *event)
{
    if (ui.imageLabel[LEFT_IMG_WIDGET]->pixMap == nullptr)
    {
        int scrollWidth = ui.scrollArea[LEFT_IMG_WIDGET]->geometry().width();
        int scrollHeight = ui.scrollArea[LEFT_IMG_WIDGET]->geometry().height();
        int scrollBarWidth = ui.scrollArea[LEFT_IMG_WIDGET]->verticalScrollBar()->width();
        int scrollBarHeight = ui.scrollArea[LEFT_IMG_WIDGET]->horizontalScrollBar()->height();
        ui.imageLabelContianer[LEFT_IMG_WIDGET]->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
        ui.imageLabelContianer[LEFT_IMG_WIDGET]->layout()->setContentsMargins(0, 0, 0, 0);
    }
    if (ui.imageLabel[RIGHT_IMG_WIDGET]->pixMap == nullptr)
    {
        int scrollWidth = ui.scrollArea[RIGHT_IMG_WIDGET]->geometry().width();
        int scrollHeight = ui.scrollArea[RIGHT_IMG_WIDGET]->geometry().height();
        int scrollBarWidth = ui.scrollArea[RIGHT_IMG_WIDGET]->verticalScrollBar()->width();
        int scrollBarHeight = ui.scrollArea[RIGHT_IMG_WIDGET]->horizontalScrollBar()->height();
        ui.imageLabelContianer[RIGHT_IMG_WIDGET]->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
        ui.imageLabelContianer[RIGHT_IMG_WIDGET]->layout()->setContentsMargins(0, 0, 0, 0);
    }
    QMainWindow::showEvent(event);
}

void IIPviewer::onUseRoiAction(bool check)
{
    if (check)
    {
        ui.useMoveToolAction->setChecked(false);
        ui.imageLabel[LEFT_IMG_WIDGET]->setMouseActionPaintRoi();
        ui.imageLabel[RIGHT_IMG_WIDGET]->setMouseActionPaintRoi();

        ui.penColorSetAction->setVisible(true);
        ui.penWidthAction->setVisible(true);
        // ui.toolBar->update();
    }
    else
    {
        ui.imageLabel[LEFT_IMG_WIDGET]->setMouseActionNone();
        ui.imageLabel[RIGHT_IMG_WIDGET]->setMouseActionNone();

        ui.penColorSetAction->setVisible(false);
        ui.penWidthAction->setVisible(false);
        // ui.toolBar->update();
    }
}

void IIPviewer::onUseMoveAction(bool check)
{
    if (check)
    {
        ui.useRoiToolAction->setChecked(false);
        ui.penColorSetAction->setVisible(false);
        ui.penWidthAction->setVisible(false);
        ui.imageLabel[LEFT_IMG_WIDGET]->setMouseActionDragImg();
        ui.imageLabel[RIGHT_IMG_WIDGET]->setMouseActionDragImg();
    }
    else
    {
        ui.imageLabel[LEFT_IMG_WIDGET]->setMouseActionNone();
        ui.imageLabel[RIGHT_IMG_WIDGET]->setMouseActionNone();
    }
}

void IIPviewer::onSingleImgModeAction(bool check)
{
    ui.workAreaDoubleModeAction->setChecked(!check);
    settings.workAreaDoubleImgMode = !check;
    if(settings.workAreaDoubleImgMode)
    {
        ui.scrollArea[1]->show();
        ui.scrollAreaCenterFrame->show();
        ui.openFileRightAction->setEnabled(true);
        ui.reloadFileRightAction->setEnabled(true);
        ui.closeRightAction->setEnabled(true);
    }
    else
    {
        ui.scrollArea[1]->hide();
        ui.scrollAreaCenterFrame->hide();
        ui.openFileRightAction->setEnabled(false);
        ui.reloadFileRightAction->setEnabled(false);
        ui.closeRightAction->setEnabled(false);
    }
    ui.mainWidget->adjustSize();
}

void IIPviewer::onDoubleImgModeAction(bool check)
{
    ui.workAreaSingleModeAction->setChecked(!check);
    settings.workAreaDoubleImgMode = check;
    if(settings.workAreaDoubleImgMode)
    {
        ui.scrollArea[1]->show();
        ui.scrollAreaCenterFrame->show();
        ui.openFileRightAction->setEnabled(true);
        ui.reloadFileRightAction->setEnabled(true);
        ui.closeRightAction->setEnabled(true);
    }
    else
    {
        ui.scrollArea[1]->hide();
        ui.scrollAreaCenterFrame->hide();
        ui.openFileRightAction->setEnabled(false);
        ui.reloadFileRightAction->setEnabled(false);
        ui.closeRightAction->setEnabled(false);
    }
    ui.mainWidget->adjustSize();
}

void IIPviewer::onSysOptionAction(bool check)
{
    Q_UNUSED(check);
    IIPOptionDialog dlg(this);
    dlg.set_uv_disp_mode(this->settings.uv_value_disp_mode);
    dlg.set_pix_val_bg_color_index(this->settings.pix_val_bg_index);
    dlg.set_pix_val_custom_bg_color(this->settings.pix_val_cus_bg_color);
    int resp = dlg.exec();
    if(resp == IIPOptionDialog::DialogCode::Accepted)
    {
        this->settings.uv_value_disp_mode = dlg.uv_disp_mode;
        this->settings.pix_val_bg_index = dlg.pix_val_bg_color_index;
        this->settings.pix_val_cus_bg_color = dlg.pix_val_cus_bg_color;
    }
}

void IIPviewer::showImageInfo()
{
    ImgInfoDlg dlg(this);
    if (openedFile[0].endsWith("raw", Qt::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], ui.imageLabel[0]->rawBayerType, YuvFileInfoDlg::YUV_UNKNOW, ui.imageLabel[0]->rawDataBit, true);
    }
    else if (openedFile[0].endsWith("pnm", Qt::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], RawFileInfoDlg::BAYER_UNKNOW, YuvFileInfoDlg::YUV_UNKNOW, ui.imageLabel[0]->pnmDataBit, true);
    }
    else if (openedFile[0].endsWith("yuv", Qt::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], RawFileInfoDlg::BAYER_UNKNOW, ui.imageLabel[0]->yuvType, ui.imageLabel[0]->yuvDataBit, true);
    }
    else if (openedFile[0].endsWith("jpg", Qt::CaseInsensitive) || openedFile[0].endsWith("jpeg", Qt::CaseInsensitive) || openedFile[0].endsWith("bmp", Qt::CaseInsensitive) || openedFile[0].endsWith("png", Qt::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[0], originSize[0], RawFileInfoDlg::BAYER_UNKNOW, YuvFileInfoDlg::YUV_UNKNOW, 8, true);
    }

    if (openedFile[1].endsWith("raw", Qt::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], ui.imageLabel[1]->rawBayerType, YuvFileInfoDlg::YUV_UNKNOW, ui.imageLabel[1]->rawDataBit, false);
    }
    else if (openedFile[1].endsWith("pnm", Qt::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], RawFileInfoDlg::BAYER_UNKNOW, YuvFileInfoDlg::YUV_UNKNOW, ui.imageLabel[1]->pnmDataBit, false);
    }
    else if (openedFile[1].endsWith("yuv", Qt::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], RawFileInfoDlg::BAYER_UNKNOW, ui.imageLabel[1]->yuvType, ui.imageLabel[1]->yuvDataBit, false);
    }
    else if (openedFile[1].endsWith("jpg", Qt::CaseInsensitive) || openedFile[1].endsWith("jpeg", Qt::CaseInsensitive) || openedFile[1].endsWith("bmp", Qt::CaseInsensitive) || openedFile[1].endsWith("png", Qt::CaseInsensitive))
    {
        dlg.setImgInfo(openedFile[1], originSize[1], RawFileInfoDlg::BAYER_UNKNOW, YuvFileInfoDlg::YUV_UNKNOW, 8, false);
    }
    dlg.exec();
}

void IIPviewer::checkUpdate()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished, [this, manager](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
             QByteArray response = reply->readAll(); 
             QJsonDocument jsonDoc = QJsonDocument::fromJson(response); 
             QJsonObject jsonObj = jsonDoc.object(); 
             QString latestVersion = jsonObj["tag_name"].toString();
             if(latestVersion.startsWith('v', Qt::CaseSensitivity::CaseInsensitive)) {
                latestVersion.remove(QChar('v'), Qt::CaseSensitivity::CaseInsensitive);
             }
             QVersionNumber currentVersion{IIViewer_VERSION_MAJOR, IIViewer_VERSION_MINOR, IIViewer_VERSION_PATCH}; // 当前版本号 
             QVersionNumber newVersion = QVersionNumber::fromString(latestVersion);
            //  qDebug() << currentVersion << newVersion << latestVersion;
             if (newVersion > currentVersion) {
                QString text("<a href=\"https://github.com/JonahZeng/IIViewer/releases\">Click here to github release page download new version</a>");
                QMessageBox msgBox(QMessageBox::Icon::Information, tr("find new version"), text, QMessageBox::StandardButton::Ok, this);
                msgBox.exec();
                // QMessageBox::information(this, tr("find new version"), tr("<a href=\"https://github.com/JonahZeng/IIViewer/releases\">Click here to github release page download new version</a>"), QMessageBox::StandardButton::Ok);
            } else { 
                QMessageBox::information(this, tr("no new version"), tr("You are using the latest version"), QMessageBox::StandardButton::Ok);
            } 
        } else {
            QMessageBox::critical(this, tr("network error"), QString("Error checking for updates: %1").arg(reply->errorString()), QMessageBox::StandardButton::Ok);
        } 
        reply->deleteLater();
        manager->deleteLater();
    }); 
    QUrl url("https://api.github.com/repos/JonahZeng/IIViewer/releases/latest");
    QNetworkRequest request(url); 
    manager->get(request);
}

void IIPviewer::openedFileChanged(const QString &filePath)
{
    if(filePath == openedFile[LEFT_IMG_WIDGET])
    {
        QDateTime curNotiyTime = QDateTime::currentDateTime();
        if(filePath == lastFileWatcherNotifyPath[0] && (lastFileWatcherNotifyTime[0].msecsTo(curNotiyTime) < 100 || lastFileWatcherNotifyIsWaitProcess[0]))
        {
            return;
        }
        else
        {
            lastFileWatcherNotifyPath[0] = filePath;
            lastFileWatcherNotifyTime[0] = curNotiyTime;
        }
        auto curModifiedTime = QFileInfo(filePath).lastModified();
        if (curModifiedTime > openedFileLastModifiedTime[LEFT_IMG_WIDGET])
        {
            lastFileWatcherNotifyIsWaitProcess[0] = true;
            auto resp = QMessageBox::information(this, tr("file changed"), filePath + tr(" has been changed, reload it?"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::No);
            if (resp == QMessageBox::StandardButton::Ok)
            {
                reLoadFile(LEFT_IMG_WIDGET);
                masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];
                openedFileLastModifiedTime[LEFT_IMG_WIDGET] = curModifiedTime;
            }
            lastFileWatcherNotifyIsWaitProcess[0] = false;
        }
    }
    else if(filePath == openedFile[RIGHT_IMG_WIDGET])
    {
        QDateTime curNotiyTime = QDateTime::currentDateTime();
        if(filePath == lastFileWatcherNotifyPath[1] && (lastFileWatcherNotifyTime[1].msecsTo(curNotiyTime) < 100 || lastFileWatcherNotifyIsWaitProcess[1]))
        {
            return;
        }
        else
        {
            lastFileWatcherNotifyPath[1] = filePath;
            lastFileWatcherNotifyTime[1] = curNotiyTime;
        }
        auto curModifiedTime = QFileInfo(filePath).lastModified();
        if (curModifiedTime > openedFileLastModifiedTime[RIGHT_IMG_WIDGET])
        {
            lastFileWatcherNotifyIsWaitProcess[1] = true;
            auto resp = QMessageBox::information(this, tr("file changed"), filePath + tr(" has been changed, reload it?"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::No);
            if (resp == QMessageBox::StandardButton::Ok)
            {
                reLoadFile(RIGHT_IMG_WIDGET);
                masterScrollarea = ui.scrollArea[RIGHT_IMG_WIDGET];
                openedFileLastModifiedTime[RIGHT_IMG_WIDGET] = curModifiedTime;
            }
            lastFileWatcherNotifyIsWaitProcess[1] = false;
        }
    }
    else
    {
        openedFileWatcher.removePath(filePath);
    }
}

void IIPviewer::updateExchangeBtn()
{
    if (ui.imageLabel[LEFT_IMG_WIDGET]->openedImgType != UNKNOW_IMG && ui.imageLabel[RIGHT_IMG_WIDGET]->openedImgType != UNKNOW_IMG)
    {
        ui.exchangeAreaPreviewBtn->setEnabled(true);
    }
    else
    {
        ui.exchangeAreaPreviewBtn->setEnabled(false);
    }
}

void IIPviewer::updateZoomLabelText()
{
    if (ui.imageLabel[LEFT_IMG_WIDGET]->openedImgType == UNKNOW_IMG && ui.imageLabel[RIGHT_IMG_WIDGET]->openedImgType == UNKNOW_IMG)
    {
        ui.zoomRatioLabel->setEnabled(false);
        ui.zoomRatioLabel->setText("1.00x");
    }
    else
    {
        ui.zoomRatioLabel->setEnabled(true);
    }
}

void IIPviewer::exchangeRight2LeftImg()
{
    ui.imageLabel[LEFT_IMG_WIDGET]->acceptImgFromOther(ui.imageLabel[RIGHT_IMG_WIDGET]);
    ui.exchangeAreaPreviewBtn->setIcon(QIcon(":image/resource/right2left-pressed.png"));
}

void IIPviewer::restoreLeftImg()
{
    ui.imageLabel[LEFT_IMG_WIDGET]->restoreImg();
    ui.exchangeAreaPreviewBtn->setIcon(QIcon(":image/resource/right2left.png"));
}

void IIPviewer::closeEvent(QCloseEvent *event)
{
    auto reply = QMessageBox::question(this, tr("Confirm"), tr("Are you sure to quit?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
    if (reply == QMessageBox::Yes)
    {
        // Save window position and screen info
        QRect geometry = this->geometry();
        QPoint center = geometry.center();
        QScreen *screen = QApplication::screenAt(center);

        if (screen)
        {
            settings.windowGeometry = geometry;
            settings.windowScreenName = screen->name();

            // Verify the screen still exists (multi-screen environment may change)
            bool screenExists = false;
            for (QScreen *s : QApplication::screens())
            {
                if (s->name() == screen->name())
                {
                    screenExists = true;
                    break;
                }
            }

            if (!screenExists)
            {
                // Fallback to primary screen if current screen not found
                settings.windowScreenName = QApplication::primaryScreen()->name();
            }
        }
        else
        {
            // No screen found at center, use primary screen
            settings.windowGeometry = geometry;
            settings.windowScreenName = QApplication::primaryScreen()->name();
        }

        onCloseLeftFileAction();
        onCloseRightFileAction();
        QMainWindow::closeEvent(event);
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void IIPviewer::setTitle()
{
    QString openedFile0_t(openedFile[0]);
    QString openedFile1_t(openedFile[1]);
    if (openedFile[0].length() > 50)
    {
        openedFile0_t = openedFile[0].left(20) + ".........." + openedFile[0].right(20);
    }
    if (openedFile[1].length() > 50)
    {
        openedFile1_t = openedFile[1].left(20) + ".........." + openedFile[1].right(20);
    }
    if (openedFile[0].isEmpty() && openedFile[1].isEmpty())
    {
        setWindowTitle("IIViewer");
    }
    else if (openedFile[0].isEmpty() || openedFile[1].isEmpty())
    {
        setWindowTitle(openedFile0_t + openedFile1_t + " - IIViewer");
    }
    else
    {
        setWindowTitle(openedFile0_t + "<--->" + openedFile1_t + " - IIViewer");
    }
}

void IIPviewer::onOpenFileAction()
{
    // QString path = settings.workPath;
    auto fileName = QFileDialog::getOpenFileName(this, tr("open images"), settings.workPath,
        "Images files(*.jpg *JPG *.jpeg *JPEG *.png *PNG *.bmp *BMP *.tif *TIF *.tiff *TIFF);;Raw files(*.raw *.RAW);;Pnm files(*.pnm *.PNM);;Pgm files(*.pgm *.PGM);;yuv files(*.yuv *.YUV);;Tiff files(*.tif *.TIF *.tiff *.TIFF);;All files(*.*)");

    if (fileName.isEmpty())
    {
        return;
    }

    QFileInfo info(fileName);
    settings.workPath = info.absolutePath();
    if (sender() == static_cast<QObject *>(ui.openFileLeftAction))
    {
        onCloseLeftFileAction(); // 这里删除关闭文件监控
        loadFile(fileName, LEFT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];
        openedFileWatcher.addPath(fileName); // 添加新文件监控
        openedFileLastModifiedTime[LEFT_IMG_WIDGET] = QFileInfo(fileName).lastModified(); // 记录新文件的修改时间
    }
    else if (sender() == static_cast<QObject *>(ui.openFileRightAction))
    {
        onCloseRightFileAction();
        loadFile(fileName, RIGHT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea[RIGHT_IMG_WIDGET];
        openedFileWatcher.addPath(fileName); // 添加新文件监控
        openedFileLastModifiedTime[RIGHT_IMG_WIDGET] = QFileInfo(fileName).lastModified(); // 记录新文件的修改时间
    }
    setTitle();

    emit updateExchangeBtnStatus();
    emit updateZoomLabelStatus();
}

/**
 * @brief 从命令行参数启动并打开图像，如果注册了文件关联，双击图像直接进入程序
 * 
 * @param image 
 */
void IIPviewer::openGivenFileFromCmdArgv(QString image)
{
    QFileInfo info(image);
    // qDebug() << info.suffix();
    QString suf = info.suffix().toLower();
    if(suf == "jpg" || suf == "png" || suf == "bmp" || suf == "tif" || suf == "tiff" || suf == "raw" || suf == "yuv" || suf == "pnm" || suf == "pgm")
    {
        onCloseLeftFileAction();
        loadFile(image, LEFT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];
        openedFileWatcher.addPath(image);
        openedFileLastModifiedTime[LEFT_IMG_WIDGET] = QFileInfo(image).lastModified();
        setTitle();
        emit updateExchangeBtnStatus();
        emit updateZoomLabelStatus();
    }
}

void IIPviewer::loadFilePostProcessLayoutAndScrollValue(int leftOrRight)
{
    int master = LEFT_IMG_WIDGET;
    int slave = RIGHT_IMG_WIDGET;
    if (leftOrRight == LEFT_IMG_WIDGET)
    {
    }
    else if (leftOrRight == RIGHT_IMG_WIDGET)
    {
        master = RIGHT_IMG_WIDGET;
        slave = LEFT_IMG_WIDGET;
    }

    if (!openedFile[slave].isEmpty())
    {
        int zoomIdx = ui.imageLabel[slave]->zoomIdx;
        if (zoomIdx > 2)
        {
            ui.imageLabel[master]->zoomIn(zoomIdx);
        }
        else if (zoomIdx < 2)
        {
            ui.imageLabel[master]->zoomOut(zoomIdx);
        }
        ui.imageLabelContianer[master]->layout()->setContentsMargins(ui.imageLabelContianer[slave]->layout()->contentsMargins());
        ui.imageLabelContianer[master]->resize(ui.imageLabelContianer[slave]->size());

        int value = ui.scrollArea[slave]->horizontalScrollBar()->value();
        ui.scrollArea[master]->horizontalScrollBar()->setValue(value);
        value = ui.scrollArea[slave]->verticalScrollBar()->value();
        ui.scrollArea[master]->verticalScrollBar()->setValue(value);
    }
    else
    {
        int scrollAreaClientWidth = ui.scrollArea[master]->width() - ui.scrollArea[master]->verticalScrollBar()->width();
        int scrollAreaClientHeight = ui.scrollArea[master]->height() - ui.scrollArea[master]->horizontalScrollBar()->height();

        ui.imageLabelContianer[master]->resize(std::max(scrollAreaClientWidth, originSize[master].width()), std::max(scrollAreaClientHeight, originSize[master].height()));
        int margin_hor = 0;
        int margin_ver = 0;
        if (scrollAreaClientWidth > originSize[master].width())
        {
            margin_hor = (scrollAreaClientWidth - originSize[master].width()) / 2;
        }
        if (scrollAreaClientHeight > originSize[0].height())
        {
            margin_ver = (scrollAreaClientHeight - originSize[master].height()) / 2;
        }
        ui.imageLabelContianer[master]->layout()->setContentsMargins(margin_hor, margin_ver, margin_hor, margin_ver);

        int hmax = ui.scrollArea[master]->horizontalScrollBar()->maximum();
        int hmin = ui.scrollArea[master]->horizontalScrollBar()->minimum();
        int vmax = ui.scrollArea[master]->verticalScrollBar()->maximum();
        int vmin = ui.scrollArea[master]->verticalScrollBar()->minimum();

        ui.scrollArea[master]->horizontalScrollBar()->setValue((hmax - hmin) / 2);
        ui.scrollArea[master]->verticalScrollBar()->setValue((vmax - vmin) / 2);
    }
}

void IIPviewer::reLoadFile(int scrollArea)
{
    QString& dstFn = openedFile[scrollArea == LEFT_IMG_WIDGET ? 0 : 1];
    if (dstFn.endsWith(".jpg", Qt::CaseInsensitive) || dstFn.endsWith(".jpeg", Qt::CaseInsensitive) || dstFn.endsWith(".png", Qt::CaseInsensitive) || dstFn.endsWith(".bmp", Qt::CaseInsensitive))
    {
        QImageReader reader(dstFn);
        reader.setAutoTransform(true);
        if (!reader.canRead())
        {
            QString t = QString("can not open ") + dstFn + QString(" as image!");
            QMessageBox::information(this, tr("error"), t, QMessageBox::StandardButton::Ok);
            return;
        }

        if (scrollArea == LEFT_IMG_WIDGET)
        {
            if (!openedFile[1].isEmpty())
            {
                if (reader.size() != originSize[1])
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            // openedFile[0] = dstFn;
            originSize[0] = reader.size();
            setImage(dstFn, LEFT_IMG_WIDGET);

            // loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
        }
        else if (scrollArea == RIGHT_IMG_WIDGET)
        {
            if (openedFile[0].length() > 0)
            {
                if (reader.size() != originSize[0])
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            // openedFile[1] = dstFn;
            originSize[1] = reader.size();
            // ui.imageLabelContianer[1]->resize(originSize[1]);
            setImage(dstFn, RIGHT_IMG_WIDGET);

            // loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
        }
    }
    else if (dstFn.endsWith(".raw", Qt::CaseInsensitive))
    {
        loadRawFile(dstFn, scrollArea, true);
    }
    else if (dstFn.endsWith(".pnm", Qt::CaseInsensitive))
    {
        loadPnmFile(dstFn, scrollArea, true);
    }
    else if (dstFn.endsWith(".pgm", Qt::CaseInsensitive))
    {
        loadPgmFile(dstFn, scrollArea, true);
    }
    else if (dstFn.endsWith(".yuv", Qt::CaseInsensitive))
    {
        loadYuvFile(dstFn, scrollArea, true);
    }
    else
    {
        QMessageBox::warning(this, tr("not support"), tr("this format file not support yet!"), QMessageBox::StandardButton::Ok);
        return;
    }
}

void IIPviewer::loadFile(QString &fileName, int scrollArea)
{
    if (fileName.endsWith(".jpg", Qt::CaseInsensitive) || 
        fileName.endsWith(".jpeg", Qt::CaseInsensitive) || 
        fileName.endsWith(".png", Qt::CaseInsensitive) || 
        fileName.endsWith(".bmp", Qt::CaseInsensitive) ||
        fileName.endsWith(".tif", Qt::CaseInsensitive) ||
        fileName.endsWith(".tiff", Qt::CaseInsensitive))
    {
        QImageReader reader(fileName);
        reader.setAutoTransform(true);
        if (!reader.canRead())
        {
            QString t = QString("can not open ") + fileName + QString(" as image!");
            QMessageBox::information(this, tr("error"), t, QMessageBox::StandardButton::Ok);
            return;
        }

        if (scrollArea == LEFT_IMG_WIDGET)
        {
            onCloseLeftFileAction();
            if (!openedFile[1].isEmpty())
            {
                if (reader.size() != originSize[1])
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            openedFile[0] = fileName;
            originSize[0] = reader.size();
            setImage(fileName, LEFT_IMG_WIDGET);

            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
        }
        else if (scrollArea == RIGHT_IMG_WIDGET)
        {
            onCloseRightFileAction();
            if (openedFile[0].length() > 0)
            {
                if (reader.size() != originSize[0])
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            openedFile[1] = fileName;
            originSize[1] = reader.size();
            // ui.imageLabelContianer[1]->resize(originSize[1]);
            setImage(fileName, RIGHT_IMG_WIDGET);

            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
        }
    }
    else if (fileName.endsWith(".raw", Qt::CaseInsensitive))
    {
        loadRawFile(fileName, scrollArea);
    }
    else if (fileName.endsWith(".pnm", Qt::CaseInsensitive))
    {
        loadPnmFile(fileName, scrollArea);
    }
    else if (fileName.endsWith(".pgm", Qt::CaseInsensitive))
    {
        loadPgmFile(fileName, scrollArea);
    }
    else if (fileName.endsWith(".yuv", Qt::CaseInsensitive))
    {
        loadYuvFile(fileName, scrollArea);
    }
    else
    {
        QMessageBox::warning(this, tr("not support"), tr("this format file not support yet!"), QMessageBox::StandardButton::Ok);
        return;
    }
}

void IIPviewer::loadYuvFile(QString &fileName, int scrollArea, bool reload)
{
    YuvFileInfoDlg dlg(this);

    auto yuvtp = settings.yuvType;
    if (yuvtp == YuvFileInfoDlg::YUV444_INTERLEAVE)
    {
        dlg.ui.formatComboBox->setCurrentIndex(0);
    }
    else if (yuvtp == YuvFileInfoDlg::YUV444_PLANAR)
    {
        dlg.ui.formatComboBox->setCurrentIndex(1);
    }
    else if (yuvtp == YuvFileInfoDlg::YUV422_UYVY)
    {
        dlg.ui.formatComboBox->setCurrentIndex(2);
    }
    else if (yuvtp == YuvFileInfoDlg::YUV422_YUYV)
    {
        dlg.ui.formatComboBox->setCurrentIndex(3);
    }
    else if (yuvtp == YuvFileInfoDlg::YUV420_NV12)
    {
        dlg.ui.formatComboBox->setCurrentIndex(4);
    }
    else if (yuvtp == YuvFileInfoDlg::YUV420_NV21)
    {
        dlg.ui.formatComboBox->setCurrentIndex(5);
    }
    else if (yuvtp == YuvFileInfoDlg::YUV420P_YU12)
    {
        dlg.ui.formatComboBox->setCurrentIndex(6);
    }
    else if (yuvtp == YuvFileInfoDlg::YUV420P_YV12)
    {
        dlg.ui.formatComboBox->setCurrentIndex(7);
    }
    else if (yuvtp == YuvFileInfoDlg::YUV400)
    {
        dlg.ui.formatComboBox->setCurrentIndex(8);
    }
    dlg.ui.bitDepthSpinBox->setValue(settings.yuv_bitDepth);
    dlg.ui.widthLineEdit->setText(QString::asprintf("%d", settings.yuv_width));
    dlg.ui.heightLineEdit->setText(QString::asprintf("%d", settings.yuv_height));

    int reply = dlg.exec();
    if (reply != YuvFileInfoDlg::Accepted)
    {
        return;
    }

    int bitDepth = dlg.ui.bitDepthSpinBox->value();
    int pixSize = 2;
    if (bitDepth <= 8)
    {
        pixSize = 1;
    }
    else if (bitDepth > 8 && bitDepth <= 16)
    {
        pixSize = 2;
    }
    else
    {
        QMessageBox::warning(this, tr("error"), tr("yuv bits > 16"), QMessageBox::StandardButton::Ok);
        return;
    }

    int width = dlg.ui.widthLineEdit->text().toInt();
    int height = dlg.ui.heightLineEdit->text().toInt();
    qint64 totalSize = 0;
    YuvFileInfoDlg::YuvType tp = YuvFileInfoDlg::YUV444_INTERLEAVE;
    int curIdx = dlg.ui.formatComboBox->currentIndex();
    if (curIdx == 0)
    {
        tp = YuvFileInfoDlg::YUV444_INTERLEAVE;
        totalSize = width * height * pixSize * 3;
    }
    else if (curIdx == 1)
    {
        tp = YuvFileInfoDlg::YUV444_PLANAR;
        totalSize = width * height * pixSize * 3;
    }
    else if (curIdx == 2)
    {
        tp = YuvFileInfoDlg::YUV422_UYVY;
        totalSize = width * height * pixSize * 2;
    }
    else if (curIdx == 3)
    {
        tp = YuvFileInfoDlg::YUV422_YUYV;
        totalSize = width * height * pixSize * 2;
    }
    else if (curIdx == 4)
    {
        tp = YuvFileInfoDlg::YUV420_NV12;
        totalSize = width * height * pixSize * 3 / 2;
    }
    else if (curIdx == 5)
    {
        tp = YuvFileInfoDlg::YUV420_NV21;
        totalSize = width * height * pixSize * 3 / 2;
    }
    else if (curIdx == 6)
    {
        tp = YuvFileInfoDlg::YUV420P_YU12;
        totalSize = width * height * pixSize * 3 / 2;
    }
    else if (curIdx == 7)
    {
        tp = YuvFileInfoDlg::YUV420P_YV12;
        totalSize = width * height * pixSize * 3 / 2;
    }
    else if (curIdx == 8)
    {
        tp = YuvFileInfoDlg::YUV400;
        totalSize = width * height * pixSize;
    }

    settings.yuvType = tp;
    settings.yuv_bitDepth = bitDepth;
    settings.yuv_width = width;
    settings.yuv_height = height;

    QFileInfo fileInfo(fileName);
    qint64 yuvFileSize = fileInfo.size();
    if (totalSize > yuvFileSize)
    {
        QMessageBox::warning(this, tr("error"), tr("yuv file size < your require"), QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        if (!openedFile[1].isEmpty())
        {
            if (QSize(width, height) != originSize[1])
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[0] = fileName;
        originSize[0] = QSize(width, height);
        setYuvImage(fileName, tp, bitDepth, width, height, pixSize, LEFT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);}
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseRightFileAction();
        }
        if (!openedFile[0].isEmpty())
        {
            if (QSize(width, height) != originSize[0])
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[1] = fileName;
        originSize[1] = QSize(width, height);
        setYuvImage(fileName, tp, bitDepth, width, height, pixSize, RIGHT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);}
    }
}

void IIPviewer::loadRawFile(QString &fileName, int scrollArea, bool reload)
{
    RawFileInfoDlg dlg(this);
    switch (settings.rawByType)
    {
    case RawFileInfoDlg::BayerPatternType::RGGB:
        dlg.ui.RGGBRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::GRBG:
        dlg.ui.GRBGRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::GBRG:
        dlg.ui.GBRGRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::BGGR:
        dlg.ui.BGGRRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::RGGIR:
        dlg.ui.RGGIRRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::BGGIR:
        dlg.ui.BGGIRRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::GRIRG:
        dlg.ui.GRIRGRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::GBIRG:
        dlg.ui.GBIRGRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::GIRRG:
        dlg.ui.GIRRGRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::GIRBG:
        dlg.ui.GIRBGRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::IRGGR:
        dlg.ui.IRGGRRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::IRGGB:
        dlg.ui.IRGGBRadioButton->setChecked(true);
        break;
    case RawFileInfoDlg::BayerPatternType::MONO:
        dlg.ui.MONORadioButton->setChecked(true);
        break;
    
    default:
        break;
    }
    
    auto byteOrder = settings.rawByteOrder;
    if (byteOrder == RawFileInfoDlg::ByteOrderType::RAW_LITTLE_ENDIAN)
    {
        dlg.ui.little_endian->setChecked(true);
    }
    else if (byteOrder == RawFileInfoDlg::ByteOrderType::RAW_BIG_ENDIAN)
    {
        dlg.ui.big_endian->setChecked(true);
    }
    if(!settings.raw_compact)
        dlg.ui.BitDepthComboBox->setCurrentText(QString::asprintf("%d", settings.raw_bitDepth));
    else
        dlg.ui.BitDepthComboBox->setCurrentText(QString::asprintf("%d-comp", settings.raw_bitDepth));

    dlg.ui.WidthLineEdit->setText(QString::asprintf("%d", settings.raw_width));
    dlg.ui.HeightLineEdit->setText(QString::asprintf("%d", settings.raw_height));
    int reply = dlg.exec();
    if (reply != RawFileInfoDlg::Accepted)
    {
        return;
    }
    RawFileInfoDlg::BayerPatternType by = RawFileInfoDlg::BayerPatternType::BAYER_UNKNOW;
    if (dlg.ui.RGGBRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::RGGB;
    }
    else if (dlg.ui.GRBGRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::GRBG;
    }
    else if (dlg.ui.GBRGRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::GBRG;
    }
    else if (dlg.ui.BGGRRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::BGGR;
    }
    else if (dlg.ui.RGGIRRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::RGGIR;
    }
    else if (dlg.ui.BGGIRRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::BGGIR;
    }
    else if (dlg.ui.GRIRGRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::GRIRG;
    }
    else if (dlg.ui.GBIRGRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::GBIRG;
    }
    else if (dlg.ui.GIRRGRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::GIRRG;
    }
    else if (dlg.ui.GIRBGRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::GIRBG;
    }
    else if (dlg.ui.IRGGRRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::IRGGR;
    }
    else if (dlg.ui.IRGGBRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::IRGGB;
    }
    else if (dlg.ui.MONORadioButton->isChecked())
    {
        by = RawFileInfoDlg::BayerPatternType::MONO;
    }

    auto order = RawFileInfoDlg::ByteOrderType::RAW_LITTLE_ENDIAN;
    if (dlg.ui.big_endian->isChecked())
    {
        order = RawFileInfoDlg::ByteOrderType::RAW_BIG_ENDIAN;
    }
    bool isInt = false;
    QString bit_option_str = dlg.ui.BitDepthComboBox->currentText();
    int bitDepth = bit_option_str.toInt(&isInt, 10);
    bool raw_compact = false;
    if(!isInt && bit_option_str.endsWith(QString("-comp"))) // 12-comp, sep by '-'
    {
        auto bit_opt_list = bit_option_str.split(QChar('-'));
        bitDepth = bit_opt_list[0].toInt();
        raw_compact = true;
    }
    int width = dlg.ui.WidthLineEdit->text().toInt();
    int height = dlg.ui.HeightLineEdit->text().toInt();

    settings.rawByType = by;
    settings.raw_bitDepth = bitDepth;
    settings.raw_width = width;
    settings.raw_height = height;
    settings.rawByteOrder = order;
    settings.raw_compact = raw_compact;

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

    QFileInfo fileInfo(fileName);
    qint64 rawFileSize = fileInfo.size();
    if(raw_compact)
    {
        if(width * height * bitDepth / 8 > rawFileSize)
        {
            QMessageBox::information(this, tr("error"), tr("raw file size < your input"), QMessageBox::StandardButton::Ok);
            return;
        }
    }
    else if ((qint64)pixSize * width * height > rawFileSize)
    {
        QMessageBox::information(this, tr("error"), tr("raw file size < your input"), QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        if (!openedFile[1].isEmpty())
        {
            if (QSize(width, height) != originSize[1])
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[0] = fileName;
        originSize[0] = QSize(width, height);
        setRawImage(fileName, by, order, bitDepth, raw_compact, width, height, LEFT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET); }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseRightFileAction();
        }
        if (!openedFile[0].isEmpty())
        {
            if (QSize(width, height) != originSize[0])
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[1] = fileName;
        originSize[1] = QSize(width, height);
        setRawImage(fileName, by, order, bitDepth, raw_compact, width, height, RIGHT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);}
    }
}

void IIPviewer::loadPnmFile(QString &fileName, int scrollArea, bool reload)
{
    QImageReader reader(fileName);
    if (!reader.canRead())
    {
        QString t = QString("can not open ") + fileName + QString(" as image!");
        QMessageBox::information(this, tr("error"), t, QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        if (!openedFile[1].isEmpty())
        {
            if (reader.size() != originSize[1])
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[0] = fileName;
        originSize[0] = reader.size();
        setImage(fileName, LEFT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);}
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseRightFileAction();
        }
        if (openedFile[0].length() > 0)
        {
            if (reader.size() != originSize[0])
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[1] = fileName;
        originSize[1] = reader.size();
        setImage(fileName, RIGHT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);}
    }
}

void IIPviewer::loadPgmFile(QString &fileName, int scrollArea, bool reload)
{
    QImageReader reader(fileName);
    if (!reader.canRead())
    {
        QString t = QString("can not open ") + fileName + QString(" as image!");
        QMessageBox::information(this, tr("error"), t, QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        if (!openedFile[1].isEmpty())
        {
            if (reader.size() != originSize[1])
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[0] = fileName;
        originSize[0] = reader.size();
        setImage(fileName, LEFT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);}
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseRightFileAction();
        }
        if (openedFile[0].length() > 0)
        {
            if (reader.size() != originSize[0])
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[1] = fileName;
        originSize[1] = reader.size();
        setImage(fileName, RIGHT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);}
    }
}

void IIPviewer::setImage(QString &imageName, int leftOrRight)
{
    ui.imageLabel[leftOrRight]->paintBegin = false;
    ui.imageLabel[leftOrRight]->paintEnd = false;
    ui.imageLabel[leftOrRight]->setPixmap(imageName);
}

void IIPviewer::setRawImage(QString &imageName, RawFileInfoDlg::BayerPatternType by, RawFileInfoDlg::ByteOrderType order, int bitDepth, bool compact, int width, int height, int leftOrRight)
{
    ui.imageLabel[leftOrRight]->paintBegin = false;
    ui.imageLabel[leftOrRight]->paintEnd = false;
    ui.imageLabel[leftOrRight]->setPixmap(imageName, by, order, bitDepth, compact, width, height);
}

void IIPviewer::setYuvImage(QString &imageName, YuvFileInfoDlg::YuvType tp, int bitDepth, int width, int height, int pixSize, int leftOrRight)
{
    ui.imageLabel[leftOrRight]->paintBegin = false;
    ui.imageLabel[leftOrRight]->paintEnd = false;
    ui.imageLabel[leftOrRight]->setPixmap(imageName, tp, bitDepth, width, height, pixSize);
}

void IIPviewer::onCloseLeftFileAction()
{
    if (ui.imageLabel[LEFT_IMG_WIDGET]->pixMap == nullptr)
    {
        return;
    }
    if(ui.imageLabel[RIGHT_IMG_WIDGET]->pixMap != nullptr)
    {
        masterScrollarea = ui.scrollArea[RIGHT_IMG_WIDGET];
    }

    ui.imageLabel[LEFT_IMG_WIDGET]->releaseBuffer();
    ui.imageLabel[LEFT_IMG_WIDGET]->paintBegin = false;
    ui.imageLabel[LEFT_IMG_WIDGET]->paintEnd = false;
    ui.imageLabel[LEFT_IMG_WIDGET]->zoomIdx = 2; // 关闭之后恢复到1.0x倍率
    ui.imageLabel[LEFT_IMG_WIDGET]->openedImgType = UNKNOW_IMG;
    ui.imageLabel[LEFT_IMG_WIDGET]->rawBayerType = RawFileInfoDlg::BayerPatternType::BAYER_UNKNOW;
    ui.imageLabel[LEFT_IMG_WIDGET]->rawDataBit = 0;
    openedFileWatcher.removePath(openedFile[0]);
    openedFile[0].clear();
    setTitle();
    ui.start_x_edit0->clear();
    ui.start_y_edit0->clear();
    ui.end_x_edit0->clear();
    ui.end_y_edit0->clear();

    int scrollWidth = ui.scrollArea[LEFT_IMG_WIDGET]->geometry().width();
    int scrollHeight = ui.scrollArea[LEFT_IMG_WIDGET]->geometry().height();
    int scrollBarWidth = ui.scrollArea[LEFT_IMG_WIDGET]->verticalScrollBar()->width();
    int scrollBarHeight = ui.scrollArea[LEFT_IMG_WIDGET]->horizontalScrollBar()->height();
    ui.imageLabelContianer[LEFT_IMG_WIDGET]->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
    ui.imageLabelContianer[LEFT_IMG_WIDGET]->layout()->setContentsMargins(0, 0, 0, 0);

    emit updateExchangeBtnStatus();
    emit updateZoomLabelStatus();
}

void IIPviewer::onCloseRightFileAction()
{
    if (ui.imageLabel[RIGHT_IMG_WIDGET]->pixMap == nullptr)
    {
        return;
    }
    if(ui.imageLabel[LEFT_IMG_WIDGET]->pixMap != nullptr)
    {
        masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];
    }

    ui.imageLabel[RIGHT_IMG_WIDGET]->releaseBuffer();
    ui.imageLabel[RIGHT_IMG_WIDGET]->paintBegin = false;
    ui.imageLabel[RIGHT_IMG_WIDGET]->paintEnd = false;
    ui.imageLabel[RIGHT_IMG_WIDGET]->zoomIdx = 2; // 关闭之后恢复到1.0x倍率
    ui.imageLabel[RIGHT_IMG_WIDGET]->openedImgType = UNKNOW_IMG;
    ui.imageLabel[RIGHT_IMG_WIDGET]->rawBayerType = RawFileInfoDlg::BayerPatternType::BAYER_UNKNOW;
    ui.imageLabel[RIGHT_IMG_WIDGET]->rawDataBit = 0;
    openedFileWatcher.removePath(openedFile[1]);
    openedFile[1].clear();
    setTitle();
    ui.start_x_edit1->clear();
    ui.start_y_edit1->clear();
    ui.end_x_edit1->clear();
    ui.end_y_edit1->clear();

    int scrollWidth = ui.scrollArea[RIGHT_IMG_WIDGET]->geometry().width();
    int scrollHeight = ui.scrollArea[RIGHT_IMG_WIDGET]->geometry().height();
    int scrollBarWidth = ui.scrollArea[RIGHT_IMG_WIDGET]->verticalScrollBar()->width();
    int scrollBarHeight = ui.scrollArea[RIGHT_IMG_WIDGET]->horizontalScrollBar()->height();
    ui.imageLabelContianer[RIGHT_IMG_WIDGET]->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
    ui.imageLabelContianer[RIGHT_IMG_WIDGET]->layout()->setContentsMargins(0, 0, 0, 0);

    emit updateExchangeBtnStatus();
    emit updateZoomLabelStatus();
}

void IIPviewer::onReloadFileAction()
{
    if (sender() == static_cast<QObject *>(ui.reloadFileLeftAction))
    {
        // loadFile(openedFile[0], LEFT_IMG_WIDGET);
        reLoadFile(LEFT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];
        openedFileLastModifiedTime[LEFT_IMG_WIDGET] = QFileInfo(openedFile[LEFT_IMG_WIDGET]).lastModified();
    }
    else if (sender() == static_cast<QObject *>(ui.reloadFileRightAction))
    {
        // loadFile(openedFile[1], RIGHT_IMG_WIDGET);
        reLoadFile(RIGHT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea[RIGHT_IMG_WIDGET];
        openedFileLastModifiedTime[RIGHT_IMG_WIDGET] = QFileInfo(openedFile[RIGHT_IMG_WIDGET]).lastModified();
    }
    // setTitle();

    emit updateExchangeBtnStatus();
    // emit updateZoomLabelStatus();
}

void IIPviewer::setPenWidth(int width)
{
    ui.imageLabel[0]->setPenWidth(width);
    ui.imageLabel[1]->setPenWidth(width);
}

void IIPviewer::setTheme()
{
    int t_idx = 0;
    auto themeKeys = QStyleFactory::keys();
    for (auto &theme : ui.themes)
    {
        if (sender() == static_cast<QObject *>(theme))
        {
            QApplication::setStyle(themeKeys[t_idx]);
            settings.theme = themeKeys[t_idx];
            theme->setChecked(true);
        }
        else
        {
            theme->setChecked(false);
        }
        ++t_idx;
    }
}

void IIPviewer::toggleDataAnalyseDockWgt(bool checked)
{
    if (checked)
    {
        ui.dataAnalyseDockWgt->show();
    }
    else
    {
        ui.dataAnalyseDockWgt->close();
    }

    QLayout *lyout = ui.mainWidget->layout();
    lyout->invalidate();
    lyout->activate();

    QResizeEvent event(this->size(), this->size());
    resizeEvent(&event);
}

void IIPviewer::togglePlayListDockWgt(bool checked)
{
    if (checked)
    {
        ui.playListDockWgt->show();
    }
    else
    {
        ui.playListDockWgt->close();
    }
    QLayout *lyout = ui.mainWidget->layout();
    lyout->invalidate();
    lyout->activate();

    QResizeEvent event(this->size(), this->size());
    resizeEvent(&event);
}

void IIPviewer::syncRightPos()
{
    ui.start_x_edit1->setText(ui.start_x_edit0->text());
    ui.start_y_edit1->setText(ui.start_y_edit0->text());
    ui.end_x_edit1->setText(ui.end_x_edit0->text());
    ui.end_y_edit1->setText(ui.end_y_edit0->text());
}

void IIPviewer::syncLeftPos()
{
    ui.start_x_edit0->setText(ui.start_x_edit1->text());
    ui.start_y_edit0->setText(ui.start_y_edit1->text());
    ui.end_x_edit0->setText(ui.end_x_edit1->text());
    ui.end_y_edit0->setText(ui.end_y_edit1->text());
}

void IIPviewer::flushPaintPosEdit0(QPointF startPt, QPointF endPt)
{
    QPoint realStartPt = startPt.toPoint();
    QPoint realEndPt = endPt.toPoint();
    ui.start_x_edit0->setText(QString::asprintf("%d", realStartPt.x()));
    ui.start_y_edit0->setText(QString::asprintf("%d", realStartPt.y()));
    ui.end_x_edit0->setText(QString::asprintf("%d", realEndPt.x()));
    ui.end_y_edit0->setText(QString::asprintf("%d", realEndPt.y()));
}

void IIPviewer::flushPaintPosEdit1(QPointF startPt, QPointF endPt)
{
    QPoint realStartPt = startPt.toPoint();
    QPoint realEndPt = endPt.toPoint();
    ui.start_x_edit1->setText(QString::asprintf("%d", realStartPt.x()));
    ui.start_y_edit1->setText(QString::asprintf("%d", realStartPt.y()));
    ui.end_x_edit1->setText(QString::asprintf("%d", realEndPt.x()));
    ui.end_y_edit1->setText(QString::asprintf("%d", realEndPt.y()));
}

void IIPviewer::handleInputPaintPos0()
{
    if (ui.imageLabel[LEFT_IMG_WIDGET]->pixMap == nullptr)
        return;
    if (ui.start_x_edit0->text().length() == 0 || ui.start_y_edit0->text().length() == 0 || ui.end_x_edit0->text().length() == 0 || ui.end_y_edit0->text().length() == 0)
        return;
    else
    {
        int maxWidth = ui.imageLabel[LEFT_IMG_WIDGET]->pixMap->width();
        int maxHeight = ui.imageLabel[LEFT_IMG_WIDGET]->pixMap->height();
        int start_x = ui.start_x_edit0->text().toInt();
        start_x = start_x >= maxWidth ? maxWidth : start_x;
        int start_y = ui.start_y_edit0->text().toInt();
        start_y = start_y >= maxHeight ? start_x : start_y;
        int end_x = ui.end_x_edit0->text().toInt();
        end_x = end_x >= maxWidth ? maxWidth : end_x;
        int end_y = ui.end_y_edit0->text().toInt();
        end_y = end_y >= maxHeight ? maxHeight : end_y;
        float scale_ratio = ui.imageLabel[0]->zoomList[ui.imageLabel[0]->zoomIdx];
        ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.paintCoordinates[0] = QPoint(start_x * scale_ratio, start_y * scale_ratio);
        ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.paintCoordinates[1] = QPoint(end_x * scale_ratio, end_y * scale_ratio);
        ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.originPaintCoordinates[0] = QPoint(start_x, start_y);
        ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.originPaintCoordinates[1] = QPoint(end_x, end_y);
        ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.originScaleRatio = 1.0f;
        ui.imageLabel[LEFT_IMG_WIDGET]->paintEnd = true;
        ui.imageLabel[LEFT_IMG_WIDGET]->repaint();
    }
}

void IIPviewer::handleInputPaintPos1()
{
    if (ui.imageLabel[RIGHT_IMG_WIDGET]->pixMap == nullptr)
        return;
    if (ui.start_x_edit1->text().length() == 0 || ui.start_y_edit1->text().length() == 0 || ui.end_x_edit1->text().length() == 0 || ui.end_y_edit1->text().length() == 0)
        return;
    else
    {
        int maxWidth = ui.imageLabel[RIGHT_IMG_WIDGET]->pixMap->width();
        int maxHeight = ui.imageLabel[RIGHT_IMG_WIDGET]->pixMap->height();
        int start_x = ui.start_x_edit1->text().toInt();
        start_x = start_x >= maxWidth ? maxWidth : start_x;
        int start_y = ui.start_y_edit1->text().toInt();
        start_y = start_y >= maxHeight ? start_x : start_y;
        int end_x = ui.end_x_edit1->text().toInt();
        end_x = end_x >= maxWidth ? maxWidth : end_x;
        int end_y = ui.end_y_edit1->text().toInt();
        end_y = end_y >= maxHeight ? maxHeight : end_y;
        float scale_ratio = ui.imageLabel[1]->zoomList[ui.imageLabel[1]->zoomIdx];
        ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.paintCoordinates[0] = QPoint(start_x * scale_ratio, start_y * scale_ratio);
        ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.paintCoordinates[1] = QPoint(end_x * scale_ratio, end_y * scale_ratio);
        ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.originPaintCoordinates[0] = QPoint(start_x, start_y);
        ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.originPaintCoordinates[1] = QPoint(end_x, end_y);
        ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.originScaleRatio = 1.0f;
        ui.imageLabel[RIGHT_IMG_WIDGET]->paintEnd = true;
        ui.imageLabel[RIGHT_IMG_WIDGET]->repaint();
    }
}

void IIPviewer::plotRgbContourf()
{
    bool leftPrepared = true;
    bool rightPrepared = true;
    if (ui.imageLabel[LEFT_IMG_WIDGET]->pixMap == nullptr)
    {
        leftPrepared = false;
    }
    if (ui.imageLabel[RIGHT_IMG_WIDGET]->pixMap == nullptr)
    {
        rightPrepared = false;
    }
    if (ui.start_x_edit0->text().isEmpty() || ui.start_y_edit0->text().isEmpty() || ui.end_x_edit0->text().isEmpty() || ui.end_y_edit0->text().isEmpty())
    {
        leftPrepared = false;
    }
    if (ui.start_x_edit1->text().isEmpty() || ui.start_y_edit1->text().isEmpty() || ui.end_x_edit1->text().isEmpty() || ui.end_y_edit1->text().isEmpty())
    {
        rightPrepared = false;
    }
    if (!leftPrepared && !rightPrepared)
    {
        return;
    }
    QPoint st0(ui.start_x_edit0->text().toInt(), ui.start_y_edit0->text().toInt());
    QPoint st1(ui.end_x_edit0->text().toInt(), ui.end_y_edit0->text().toInt());

    DataVisualDialog *dlg = new DataVisualDialog(this, leftPrepared, ui.imageLabel[LEFT_IMG_WIDGET], st0, st1);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
    return;
}

void IIPviewer::plotRgbHist()
{
}

void IIPviewer::selectPenPaintColor()
{
    QColorDialog dlg(penColor, this);
    int reply = dlg.exec();
    if (reply == QColorDialog::DialogCode::Accepted)
    {
        penColor = dlg.selectedColor();
        ui.imageLabel[LEFT_IMG_WIDGET]->setPaintPenColor(penColor);
        ui.imageLabel[RIGHT_IMG_WIDGET]->setPaintPenColor(penColor);
    }
    QPixmap penColorBtnIcon(32, 32);
    penColorBtnIcon.fill(penColor);
    ui.penColorSetBtn->setIcon(penColorBtnIcon);
}

void IIPviewer::zoomIn0(int zoomIdx)
{
    masterScrollarea = ui.scrollArea[1];
    if (zoomIdx <= ZOOM_LIST_LENGTH - 1 && zoomIdx >= 0)
    {
        float ra = ui.imageLabel[0]->zoomList[zoomIdx];
        ui.zoomRatioLabel->setText(QString::asprintf("%.2fx", ra));
    }
    if (ui.imageLabel[0]->pixMap != nullptr)
    {
        ui.imageLabel[0]->zoomIn(zoomIdx);
        ui.imageLabelContianer[0]->resize(ui.imageLabelContianer[1]->size());
        ui.imageLabelContianer[0]->layout()->setContentsMargins(ui.imageLabelContianer[1]->layout()->contentsMargins());
    }
}

/**
 * @brief 由left widget触发的信号，right widget响应
 * 
 * @param zoomIdx 
 */
void IIPviewer::zoomIn1(int zoomIdx)
{
    masterScrollarea = ui.scrollArea[0];
    if (zoomIdx <= ZOOM_LIST_LENGTH - 1 && zoomIdx >= 0)
    {
        float ra = ui.imageLabel[0]->zoomList[zoomIdx];
        ui.zoomRatioLabel->setText(QString::asprintf("%.2fx", ra));
    }
    if (ui.imageLabel[1]->pixMap != nullptr)
    {
        ui.imageLabel[1]->zoomIn(zoomIdx);
        ui.imageLabelContianer[1]->resize(ui.imageLabelContianer[0]->size());
        ui.imageLabelContianer[1]->layout()->setContentsMargins(ui.imageLabelContianer[0]->layout()->contentsMargins());
    }
}

/**
 * @brief 由right widget触发的信号，left widget响应
 * 
 * @param zoomIdx 
 */
void IIPviewer::zoomOut0(int zoomIdx)
{
    masterScrollarea = ui.scrollArea[1];
    if (zoomIdx <= ZOOM_LIST_LENGTH - 1 && zoomIdx >= 0)
    {
        float ra = ui.imageLabel[0]->zoomList[zoomIdx];
        ui.zoomRatioLabel->setText(QString::asprintf("%.2fx", ra));
    }
    if (ui.imageLabel[0]->pixMap != nullptr)
    {
        ui.imageLabel[0]->zoomOut(zoomIdx);
        ui.imageLabelContianer[0]->resize(ui.imageLabelContianer[1]->size());
        ui.imageLabelContianer[0]->layout()->setContentsMargins(ui.imageLabelContianer[1]->layout()->contentsMargins());
    }
}

void IIPviewer::zoomOut1(int zoomIdx)
{
    masterScrollarea = ui.scrollArea[0];
    if (zoomIdx <= ZOOM_LIST_LENGTH - 1 && zoomIdx >= 0)
    {
        float ra = ui.imageLabel[0]->zoomList[zoomIdx];
        ui.zoomRatioLabel->setText(QString::asprintf("%.2fx", ra));
    }
    if (ui.imageLabel[1]->pixMap != nullptr)
    {
        ui.imageLabel[1]->zoomOut(zoomIdx);
        ui.imageLabelContianer[1]->resize(ui.imageLabelContianer[0]->size());
        ui.imageLabelContianer[1]->layout()->setContentsMargins(ui.imageLabelContianer[0]->layout()->contentsMargins());
    }
}

void IIPviewer::clearPaint()
{
    ui.start_x_edit1->clear();
    ui.start_y_edit1->clear();
    ui.end_x_edit1->clear();
    ui.end_y_edit1->clear();
    ui.start_x_edit0->clear();
    ui.start_y_edit0->clear();
    ui.end_x_edit0->clear();
    ui.end_y_edit0->clear();
    ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.paintCoordinates[0] = QPoint();
    ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.paintCoordinates[1] = QPoint();
    ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.originPaintCoordinates[0] = QPoint();
    ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.originPaintCoordinates[1] = QPoint();
    ui.imageLabel[LEFT_IMG_WIDGET]->ptCodInfo.originScaleRatio = 1.0f;
    ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.paintCoordinates[0] = QPoint();
    ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.paintCoordinates[1] = QPoint();
    ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.originPaintCoordinates[0] = QPoint();
    ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.originPaintCoordinates[1] = QPoint();
    ui.imageLabel[RIGHT_IMG_WIDGET]->ptCodInfo.originScaleRatio = 1.0f;
    ui.imageLabel[LEFT_IMG_WIDGET]->repaint();
    ui.imageLabel[RIGHT_IMG_WIDGET]->repaint();
}

void IIPviewer::handleRightMouseBtnDrag0(QPointF startPt, QPointF endPt)
{
    if (!ui.scrollArea[0]->horizontalScrollBar()->isVisible() && !ui.scrollArea[0]->verticalScrollBar()->isVisible())
        return;
    masterScrollarea = ui.scrollArea[0];
    int curHscrollVal = ui.scrollArea[0]->horizontalScrollBar()->value();
    int delta_x = int(startPt.x() - endPt.x());
    int curVscrollVal = ui.scrollArea[0]->verticalScrollBar()->value();
    int delta_y = int(startPt.y() - endPt.y());
    ui.scrollArea[0]->horizontalScrollBar()->setValue(curHscrollVal + delta_x);
    ui.scrollArea[0]->verticalScrollBar()->setValue(curVscrollVal + delta_y);
}

void IIPviewer::handleRightMouseBtnDrag1(QPointF startPt, QPointF endPt)
{
    if (!ui.scrollArea[1]->horizontalScrollBar()->isVisible() && !ui.scrollArea[1]->verticalScrollBar()->isVisible())
        return;
    masterScrollarea = ui.scrollArea[1];
    int curHscrollVal = ui.scrollArea[1]->horizontalScrollBar()->value();
    int delta_x = int(startPt.x() - endPt.x());
    int curVscrollVal = ui.scrollArea[1]->verticalScrollBar()->value();
    int delta_y = int(startPt.y() - endPt.y());
    ui.scrollArea[1]->horizontalScrollBar()->setValue(curHscrollVal + delta_x);
    ui.scrollArea[1]->verticalScrollBar()->setValue(curVscrollVal + delta_y);
}

/**
 * @brief 同步left image widget horizontal bar value
 * 
 * @param value horizontal bar value
 */
void IIPviewer::syncScrollArea1_horScBarVal(int value)
{
    ui.imageLabel[0]->update(ui.imageLabel[0]->zoomTextRect.toRect());
    ui.imageLabel[0]->update(ui.imageLabel[0]->pixValPaintRect.toRect());
    if (ui.imageLabel[1]->pixMap == nullptr)
        return;
    if (masterScrollarea == ui.scrollArea[1])
        return;

    ui.imageLabelContianer[1]->layout()->setContentsMargins(ui.imageLabelContianer[0]->layout()->contentsMargins());
    ui.imageLabelContianer[1]->resize(ui.imageLabelContianer[0]->size());
    ui.scrollArea[1]->horizontalScrollBar()->setValue(value);
}

/**
 * @brief 同步left image widget vertical bar value
 * 
 * @param value vertical bar value
 */
void IIPviewer::syncScrollArea1_verScBarVal(int value)
{
    ui.imageLabel[0]->update(ui.imageLabel[0]->zoomTextRect.toRect());
    ui.imageLabel[0]->update(ui.imageLabel[0]->pixValPaintRect.toRect());
    if (ui.imageLabel[1]->pixMap == nullptr)
        return;
    if (masterScrollarea == ui.scrollArea[1])
        return;

    ui.imageLabelContianer[1]->layout()->setContentsMargins(ui.imageLabelContianer[0]->layout()->contentsMargins());
    ui.imageLabelContianer[1]->resize(ui.imageLabelContianer[0]->size());
    ui.scrollArea[1]->verticalScrollBar()->setValue(value);
}

void IIPviewer::syncScrollArea0_horScBarVal(int value)
{
    ui.imageLabel[1]->update(ui.imageLabel[1]->zoomTextRect.toRect());
    ui.imageLabel[1]->update(ui.imageLabel[1]->pixValPaintRect.toRect());
    if (ui.imageLabel[0]->pixMap == nullptr)
        return;
    if (masterScrollarea == ui.scrollArea[0])
        return;

    ui.imageLabelContianer[0]->layout()->setContentsMargins(ui.imageLabelContianer[1]->layout()->contentsMargins());
    ui.imageLabelContianer[0]->resize(ui.imageLabelContianer[1]->size());
    ui.scrollArea[0]->horizontalScrollBar()->setValue(value);
}

void IIPviewer::syncScrollArea0_verScBarVal(int value)
{
    ui.imageLabel[1]->update(ui.imageLabel[1]->zoomTextRect.toRect());
    ui.imageLabel[1]->update(ui.imageLabel[1]->pixValPaintRect.toRect());
    if (ui.imageLabel[0]->pixMap == nullptr)
        return;
    if (masterScrollarea == ui.scrollArea[0])
        return;

    ui.imageLabelContianer[0]->layout()->setContentsMargins(ui.imageLabelContianer[1]->layout()->contentsMargins());
    ui.imageLabelContianer[0]->resize(ui.imageLabelContianer[1]->size());
    ui.scrollArea[0]->verticalScrollBar()->setValue(value);
}

void IIPviewer::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
    else
        event->ignore();
}

void IIPviewer::dropEvent(QDropEvent *event)
{
    QRect mainWidget_rect = ui.mainWidget->geometry();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPointF dropPt = event->posF();
#else
    QPointF dropPt = event->position();
#endif
    if (dropPt.x() < mainWidget_rect.left() || dropPt.x() > mainWidget_rect.right() || dropPt.y() < mainWidget_rect.top() || dropPt.y() > mainWidget_rect.bottom())
        return;
    const QMimeData *mData = event->mimeData();
    if (!mData->hasUrls())
        return;
    auto urlList = mData->urls();
    if (urlList.length() == 1)
    {
        auto scrollArea0_rect = ui.scrollArea[LEFT_IMG_WIDGET]->geometry().translated(mainWidget_rect.topLeft());
        auto scrollArea1_rect = ui.scrollArea[RIGHT_IMG_WIDGET]->geometry().translated(mainWidget_rect.topLeft());
        if (scrollArea0_rect.left() < dropPt.x() && dropPt.x() < scrollArea0_rect.right())
        {
            auto fileName0 = urlList[0].toLocalFile();
            if (fileName0.isEmpty())
                return;
            loadFile(fileName0, LEFT_IMG_WIDGET); // 这里删除关闭文件监控
            setTitle();
            masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];
            openedFileWatcher.addPath(fileName0); // 添加新文件监控
            openedFileLastModifiedTime[LEFT_IMG_WIDGET] = QFileInfo(fileName0).lastModified(); // 记录新文件的修改时间
        }
        else if (scrollArea1_rect.left() < dropPt.x() && dropPt.x() < scrollArea1_rect.right())
        {
            auto fileName0 = urlList[0].toLocalFile();
            if (fileName0.isEmpty())
                return;
            loadFile(fileName0, RIGHT_IMG_WIDGET); // 这里删除关闭文件监控
            setTitle();
            masterScrollarea = ui.scrollArea[RIGHT_IMG_WIDGET];
            openedFileWatcher.addPath(fileName0); // 添加新文件监控
            openedFileLastModifiedTime[RIGHT_IMG_WIDGET] = QFileInfo(fileName0).lastModified(); // 记录新文件的修改时间
        }
        QFileInfo info(urlList[0].toLocalFile());
        settings.workPath = info.absolutePath();
    }
    else
    {
        QMessageBox::critical(this, tr("error"), tr("At most 1 image!"), QMessageBox::StandardButton::Ok);
    }
    emit updateExchangeBtnStatus();
    emit updateZoomLabelStatus();
}

bool IIPviewer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.dataAnalyseDockWgt && event->type() == QEvent::Close)
    {
        ui.dataAnalyseAction->setChecked(false);
        return true;
    }
    if (obj == ui.playListDockWgt && event->type() == QEvent::Close)
    {
        ui.playListAction->setChecked(false);
        return true;
    }
    if (obj == ui.mainWidget && event->type() == QEvent::Resize)
    {
        if (ui.imageLabel[LEFT_IMG_WIDGET]->pixMap == nullptr)
        {
            int scrollWidth = ui.scrollArea[LEFT_IMG_WIDGET]->geometry().width();
            int scrollHeight = ui.scrollArea[LEFT_IMG_WIDGET]->geometry().height();
            int scrollBarWidth = ui.scrollArea[LEFT_IMG_WIDGET]->verticalScrollBar()->width();
            int scrollBarHeight = ui.scrollArea[LEFT_IMG_WIDGET]->horizontalScrollBar()->height();
            ui.imageLabelContianer[LEFT_IMG_WIDGET]->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
            ui.imageLabelContianer[LEFT_IMG_WIDGET]->layout()->setContentsMargins(0, 0, 0, 0);
        }

        if (ui.imageLabel[RIGHT_IMG_WIDGET]->pixMap == nullptr)
        {
            int scrollWidth = ui.scrollArea[RIGHT_IMG_WIDGET]->geometry().width();
            int scrollHeight = ui.scrollArea[RIGHT_IMG_WIDGET]->geometry().height();
            int scrollBarWidth = ui.scrollArea[RIGHT_IMG_WIDGET]->verticalScrollBar()->width();
            int scrollBarHeight = ui.scrollArea[RIGHT_IMG_WIDGET]->horizontalScrollBar()->height();
            ui.imageLabelContianer[RIGHT_IMG_WIDGET]->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
            ui.imageLabelContianer[RIGHT_IMG_WIDGET]->layout()->setContentsMargins(0, 0, 0, 0);
        }
        //        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

IIPviewer::~IIPviewer()
{
    settings.dumpSettingsToFile();
}
