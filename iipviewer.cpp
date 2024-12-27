#include "iipviewer.h"
#include "AboutDlg.h"
#include "DataVisualDlg.h"
#include "config.h"
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

constexpr int LEFT_IMG_WIDGET = 0;
constexpr int RIGHT_IMG_WIDGET = 1;

class ImgInfoDlg : public QDialog
{
public:
    ImgInfoDlg(QWidget *parent)
        : QDialog(parent)
    {
        leftLabel = new QLabel();
        rightLabel = new QLabel();
        QFrame *centerLine = new QFrame();
        centerLine->setFrameShape(QFrame::Shape::VLine);
        centerLine->setFrameShadow(QFrame::Shadow::Sunken);

        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget(leftLabel);
        layout->addWidget(centerLine);
        layout->addWidget(rightLabel);
        setLayout(layout);
        resize(400, 200);
        setWindowTitle("image info");
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
    : QMainWindow(parent), ui(), penColor(0, 0, 0), originSize{{0, 0}, {0, 0}}, openedFile{QString(), QString()}
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

    setAcceptDrops(true);
    ui.setupUi(this);
    QList<QScreen *> screenInfoList = QApplication::screens();
    QRect screenRect = screenInfoList.at(0)->geometry();
    setGeometry(screenRect.width() / 6, screenRect.height() / 6, screenRect.width() * 2 / 3, screenRect.height() * 2 / 3);
    setTitle();

    masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];

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

    ui.dataAnalyseDockWgt->installEventFilter(this);
    ui.playListDockWgt->installEventFilter(this);
    ui.mainWidget->installEventFilter(this);

    ui.dataAnalyseDockWgt->hide();
    ui.playListDockWgt->hide();
    ui.dataAnalyseAction->setChecked(false);
    ui.playListAction->setChecked(false);
    ui.exchangeAreaPreviewBtn->setEnabled(false);
    setWindowIcon(QIcon(":image/resource/aboutlog.png"));

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
                QMessageBox msgBox(QMessageBox::Icon::Information, QString("find new version"), text, QMessageBox::StandardButton::Ok, this);
                // msgBox.resize(200, 100);
                // 使用 HTML 格式化文本 

                // msgBox.setInformativeText(text);
                msgBox.exec();
            } else { 
                QMessageBox::information(this, QString("no new version"), QString("You are using the latest version"), QMessageBox::StandardButton::Ok);
            } 
        } else {
            QMessageBox::critical(this, QString("network error"), QString("Error checking for updates: %1").arg(reply->errorString()), QMessageBox::StandardButton::Ok);
        } 
        reply->deleteLater();
        manager->deleteLater();
    }); 
    QUrl url("https://api.github.com/repos/JonahZeng/IIViewer/releases/latest");
    QNetworkRequest request(url); 
    manager->get(request);
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
    auto reply = QMessageBox::question(this, "Confirm", "Are you sure to quit ? ", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
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
        setWindowTitle("IIP viewer");
    }
    else if (openedFile[0].isEmpty() || openedFile[1].isEmpty())
    {
        setWindowTitle(openedFile0_t + openedFile1_t + " - IIP viewer");
    }
    else
    {
        setWindowTitle(openedFile0_t + "<--->" + openedFile1_t + " - IIP viewer");
    }
}

void IIPviewer::onOpenFileAction()
{
    // QString path = settings.workPath;
    auto fileName = QFileDialog::getOpenFileName(this, "open images", settings.workPath,
                                                 "Images files(*.jpg *JPG *.jpeg *JPEG *.png *PNG *.bmp *BMP);;Raw files(*.raw *.RAW);;Pnm files(*.pnm *.PNM);;Pgm files(*.pgm *.PGM);;yuv files(*.yuv *.YUV);;All files(*.*)");

    if (fileName.isEmpty())
    {
        return;
    }

    QFileInfo info(fileName);
    settings.workPath = info.absolutePath();
    if (sender() == static_cast<QObject *>(ui.openFileLeftAction))
    {
        loadFile(fileName, LEFT_IMG_WIDGET);
    }
    else if (sender() == static_cast<QObject *>(ui.openFileRightAction))
    {
        loadFile(fileName, RIGHT_IMG_WIDGET);
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
    if(suf == "jpg" || suf == "png" || suf == "bmp" || suf == "raw" || suf == "yuv" || suf == "pnm" || suf == "pgm")
    {
        onCloseLeftFileAction();
        loadFile(image, LEFT_IMG_WIDGET);
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
            QMessageBox::information(this, "error", t, QMessageBox::StandardButton::Ok);
            return;
        }

        if (scrollArea == LEFT_IMG_WIDGET)
        {
            if (!openedFile[1].isEmpty())
            {
                if (reader.size() != originSize[1])
                {
                    QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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
                    QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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
        QMessageBox::warning(this, "not support", "this format file not support yet!", QMessageBox::StandardButton::Ok);
        return;
    }
}

void IIPviewer::loadFile(QString &fileName, int scrollArea)
{
    if (fileName.endsWith(".jpg", Qt::CaseInsensitive) || fileName.endsWith(".jpeg", Qt::CaseInsensitive) || fileName.endsWith(".png", Qt::CaseInsensitive) || fileName.endsWith(".bmp", Qt::CaseInsensitive))
    {
        QImageReader reader(fileName);
        reader.setAutoTransform(true);
        if (!reader.canRead())
        {
            QString t = QString("can not open ") + fileName + QString(" as image!");
            QMessageBox::information(this, "error", t, QMessageBox::StandardButton::Ok);
            return;
        }

        if (scrollArea == LEFT_IMG_WIDGET)
        {
            if (!openedFile[1].isEmpty())
            {
                if (reader.size() != originSize[1])
                {
                    QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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
            if (openedFile[0].length() > 0)
            {
                if (reader.size() != originSize[0])
                {
                    QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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
        QMessageBox::warning(this, "not support", "this format file not support yet!", QMessageBox::StandardButton::Ok);
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
        QMessageBox::warning(this, "error", "yuv bits > 16", QMessageBox::StandardButton::Ok);
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

    settings.yuvType = tp;
    settings.yuv_bitDepth = bitDepth;
    settings.yuv_width = width;
    settings.yuv_height = height;

    QFileInfo fileInfo(fileName);
    qint64 yuvFileSize = fileInfo.size();
    if (totalSize > yuvFileSize)
    {
        QMessageBox::warning(this, "error", "yuv file size < your require", QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile[1].isEmpty())
        {
            if (QSize(width, height) != originSize[1])
            {
                QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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
        if (!openedFile[0].isEmpty())
        {
            if (QSize(width, height) != originSize[0])
            {
                QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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

    auto rawBayer = settings.rawByType;
    if (rawBayer == RawFileInfoDlg::RGGB)
    {
        dlg.ui.RGGBRadioButton->setChecked(true);
    }
    else if (rawBayer == RawFileInfoDlg::GRBG)
    {
        dlg.ui.GRBGRadioButton->setChecked(true);
    }
    else if (rawBayer == RawFileInfoDlg::GBRG)
    {
        dlg.ui.GBRGRadioButton->setChecked(true);
    }
    else if (rawBayer == RawFileInfoDlg::BGGR)
    {
        dlg.ui.BGGRRadioButton->setChecked(true);
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
    dlg.ui.BitDepthComboBox->setCurrentText(QString::asprintf("%d", settings.raw_bitDepth));
    dlg.ui.WidthLineEdit->setText(QString::asprintf("%d", settings.raw_width));
    dlg.ui.HeightLineEdit->setText(QString::asprintf("%d", settings.raw_height));
    int reply = dlg.exec();
    if (reply != RawFileInfoDlg::Accepted)
    {
        return;
    }
    RawFileInfoDlg::BayerPatternType by = RawFileInfoDlg::GRBG;
    if (dlg.ui.RGGBRadioButton->isChecked())
    {
        by = RawFileInfoDlg::RGGB;
    }
    else if (dlg.ui.GRBGRadioButton->isChecked())
    {
        by = RawFileInfoDlg::GRBG;
    }
    else if (dlg.ui.GBRGRadioButton->isChecked())
    {
        by = RawFileInfoDlg::GBRG;
    }
    else if (dlg.ui.BGGRRadioButton->isChecked())
    {
        by = RawFileInfoDlg::BGGR;
    }
    auto order = RawFileInfoDlg::ByteOrderType::RAW_LITTLE_ENDIAN;
    if (dlg.ui.big_endian->isChecked())
    {
        order = RawFileInfoDlg::ByteOrderType::RAW_BIG_ENDIAN;
    }
    int bitDepth = dlg.ui.BitDepthComboBox->currentText().toInt();
    int width = dlg.ui.WidthLineEdit->text().toInt();
    int height = dlg.ui.HeightLineEdit->text().toInt();

    settings.rawByType = by;
    settings.raw_bitDepth = bitDepth;
    settings.raw_width = width;
    settings.raw_height = height;
    settings.rawByteOrder = order;

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
    if ((qint64)pixSize * width * height > rawFileSize)
    {
        QMessageBox::information(this, "error", "raw file size < your input", QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile[1].isEmpty())
        {
            if (QSize(width, height) != originSize[1])
            {
                QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[0] = fileName;
        originSize[0] = QSize(width, height);
        setRawImage(fileName, by, order, bitDepth, width, height, LEFT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET); }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (!openedFile[0].isEmpty())
        {
            if (QSize(width, height) != originSize[0])
            {
                QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
                return;
            }
        }
        openedFile[1] = fileName;
        originSize[1] = QSize(width, height);
        setRawImage(fileName, by, order, bitDepth, width, height, RIGHT_IMG_WIDGET);
        if(!reload) { loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);}
    }
}

void IIPviewer::loadPnmFile(QString &fileName, int scrollArea, bool reload)
{
    QImageReader reader(fileName);
    if (!reader.canRead())
    {
        QString t = QString("can not open ") + fileName + QString(" as image!");
        QMessageBox::information(this, "error", t, QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile[1].isEmpty())
        {
            if (reader.size() != originSize[1])
            {
                QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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
        if (openedFile[0].length() > 0)
        {
            if (reader.size() != originSize[0])
            {
                QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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
        QMessageBox::information(this, "error", t, QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile[1].isEmpty())
        {
            if (reader.size() != originSize[1])
            {
                QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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
        if (openedFile[0].length() > 0)
        {
            if (reader.size() != originSize[0])
            {
                QMessageBox::warning(this, "warning", "image0 size != image1 size", QMessageBox::StandardButton::Ok);
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

void IIPviewer::setRawImage(QString &imageName, RawFileInfoDlg::BayerPatternType by, RawFileInfoDlg::ByteOrderType order, int bitDepth, int width, int height, int leftOrRight)
{
    ui.imageLabel[leftOrRight]->paintBegin = false;
    ui.imageLabel[leftOrRight]->paintEnd = false;
    ui.imageLabel[leftOrRight]->setPixmap(imageName, by, order, bitDepth, width, height);
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
    }
    else if (sender() == static_cast<QObject *>(ui.reloadFileRightAction))
    {
        // loadFile(openedFile[1], RIGHT_IMG_WIDGET);
        reLoadFile(RIGHT_IMG_WIDGET);
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
    if (ui.imageLabel[0]->pixMap == nullptr)
        return;
    if (ui.start_x_edit0->text().length() == 0 || ui.start_y_edit0->text().length() == 0 || ui.end_x_edit0->text().length() == 0 || ui.end_y_edit0->text().length() == 0)
        return;
    else
    {
        int maxWidth = ui.imageLabel[0]->pixMap->width();
        int maxHeight = ui.imageLabel[0]->pixMap->height();
        int start_x = ui.start_x_edit0->text().toInt();
        start_x = start_x >= maxWidth ? maxWidth : start_x;
        int start_y = ui.start_y_edit0->text().toInt();
        start_y = start_y >= maxHeight ? start_x : start_y;
        int end_x = ui.end_x_edit0->text().toInt();
        end_x = end_x >= maxWidth ? maxWidth : end_x;
        int end_y = ui.end_y_edit0->text().toInt();
        end_y = end_y >= maxHeight ? maxHeight : end_y;
        float scale_ratio = ui.imageLabel[0]->zoomList[ui.imageLabel[0]->zoomIdx];
        ui.imageLabel[0]->paintCoordinates[0] = QPoint(start_x * scale_ratio, start_y * scale_ratio);
        ui.imageLabel[0]->paintCoordinates[1] = QPoint(end_x * scale_ratio, end_y * scale_ratio);
        ui.imageLabel[0]->paintEnd = true;
        ui.imageLabel[0]->repaint();
    }
}

void IIPviewer::handleInputPaintPos1()
{
    if (ui.imageLabel[1]->pixMap == nullptr)
        return;
    if (ui.start_x_edit1->text().length() == 0 || ui.start_y_edit1->text().length() == 0 || ui.end_x_edit1->text().length() == 0 || ui.end_y_edit1->text().length() == 0)
        return;
    else
    {
        int maxWidth = ui.imageLabel[1]->pixMap->width();
        int maxHeight = ui.imageLabel[1]->pixMap->height();
        int start_x = ui.start_x_edit1->text().toInt();
        start_x = start_x >= maxWidth ? maxWidth : start_x;
        int start_y = ui.start_y_edit1->text().toInt();
        start_y = start_y >= maxHeight ? start_x : start_y;
        int end_x = ui.end_x_edit1->text().toInt();
        end_x = end_x >= maxWidth ? maxWidth : end_x;
        int end_y = ui.end_y_edit1->text().toInt();
        end_y = end_y >= maxHeight ? maxHeight : end_y;
        float scale_ratio = ui.imageLabel[1]->zoomList[ui.imageLabel[1]->zoomIdx];
        ui.imageLabel[1]->paintCoordinates[0] = QPoint(start_x * scale_ratio, start_y * scale_ratio);
        ui.imageLabel[1]->paintCoordinates[1] = QPoint(end_x * scale_ratio, end_y * scale_ratio);
        ui.imageLabel[1]->paintEnd = true;
        ui.imageLabel[1]->repaint();
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
    ui.imageLabel[0]->paintCoordinates[0] = QPoint();
    ui.imageLabel[0]->paintCoordinates[1] = QPoint();
    ui.imageLabel[1]->paintCoordinates[0] = QPoint();
    ui.imageLabel[1]->paintCoordinates[1] = QPoint();
    ui.imageLabel[0]->repaint();
    ui.imageLabel[1]->repaint();
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
            loadFile(fileName0, LEFT_IMG_WIDGET);
            setTitle();
            masterScrollarea = ui.scrollArea[LEFT_IMG_WIDGET];
        }
        else if (scrollArea1_rect.left() < dropPt.x() && dropPt.x() < scrollArea1_rect.right())
        {
            auto fileName0 = urlList[0].toLocalFile();
            if (fileName0.isEmpty())
                return;
            loadFile(fileName0, RIGHT_IMG_WIDGET);
            setTitle();
            masterScrollarea = ui.scrollArea[RIGHT_IMG_WIDGET];
        }
        QFileInfo info(urlList[0].toLocalFile());
        settings.workPath = info.absolutePath();
    }
    else
    {
        QMessageBox::critical(this, "error", "At most 1 image!", QMessageBox::StandardButton::Ok);
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
