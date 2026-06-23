#include "config.h"
#include "IIViewer.h"
#include "AboutDlg.h"
#include "DataVisualDlg.h"
#include "IIVOptionDialog.h"
#include <QApplication>
#include <QColorDialog>
#include <QCursor>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHoverEvent>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeData>
#include <QMenuBar>
#include <QScreen>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QToolTip>
#ifdef Q_OS_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>
#endif
#if (defined _MSC_VER) && (defined HAVE_VISIBILITY)
#undef HAVE_VISIBILITY
#endif
#include <libheif/heif.h>

constexpr int LEFT_IMG_WIDGET = 0;
constexpr int RIGHT_IMG_WIDGET = 1;
constexpr int BIT8 = 8;
constexpr int BIT16 = 16;
// constexpr int BIT24 = 24;
constexpr int BIT32 = 32;
static QFont resolveUiDisplayFont(const AppSettings &settings)
{
    QFont font = QApplication::font();
    if (!settings.uiFontFamily.isEmpty())
    {
        font.setFamily(settings.uiFontFamily);
    }
    if (settings.uiFontPointSize > 0)
    {
        font.setPointSize(settings.uiFontPointSize);
    }
    return font;
}

#ifdef Q_OS_WINDOWS
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

#ifndef DWMWA_COLOR_NONE
#define DWMWA_COLOR_NONE 0xFFFFFFFE
#endif

static bool isWindows11OrGreater()
{
    static int result = -1;
    if (result >= 0)
        return result == 1;
    result = 0;
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll != nullptr)
    {
        using RtlGetVersionPtr = NTSTATUS (WINAPI *)(PRTL_OSVERSIONINFOW);
        auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (RtlGetVersion != nullptr)
        {
            RTL_OSVERSIONINFOW ver = {};
            ver.dwOSVersionInfoSize = sizeof(ver);
            if (RtlGetVersion(&ver) == 0)
            {
                if (ver.dwMajorVersion == 10 && ver.dwMinorVersion == 0 && ver.dwBuildNumber >= 22000)
                {
                    result = 1;
                }
            }
        }
    }
    return result == 1;
}

static int frameBorderWidth(HWND hwnd)
{
    const UINT dpi = GetDpiForWindow(hwnd);
    return GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

static int frameBorderHeight(HWND hwnd)
{
    const UINT dpi = GetDpiForWindow(hwnd);
    return GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

static void updateNativeShadow(HWND hwnd, bool maximized)
{
    BOOL compositionEnabled = FALSE;
    if (FAILED(DwmIsCompositionEnabled(&compositionEnabled)) || compositionEnabled == FALSE)
    {
        return;
    }

    COLORREF borderColor = DWMWA_COLOR_NONE;
    if (isWindows11OrGreater())
    {
        DWORD colorizationColor = 0;
        BOOL opaqueBlend = FALSE;
        if (SUCCEEDED(DwmGetColorizationColor(&colorizationColor, &opaqueBlend)))
        {
            borderColor = opaqueBlend ? colorizationColor : colorizationColor & 0x00FFFFFF;
        }
    }
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));

    MARGINS margins = {0, 0, 0, 0};
    if (!maximized && isWindows11OrGreater())
    {
        margins.cyTopHeight = 1;
    }
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

static bool isCaptionButtonHitTest(const WPARAM wParam)
{
    return wParam == HTMAXBUTTON || wParam == HTMINBUTTON || wParam == HTCLOSE;
}

static QPushButton *captionButtonFromHitTest(const WPARAM wParam, TitleBar *titleBar)
{
    if (titleBar == nullptr)
        return nullptr;
    switch (wParam)
    {
    case HTMAXBUTTON: return titleBar->getMaximizeButton();
    case HTMINBUTTON: return titleBar->getMinimizeButton();
    case HTCLOSE:     return titleBar->getCloseButton();
    default:          return nullptr;
    }
}
#endif

namespace
{
constexpr int baseToolBarHeight = 34;
constexpr int baseToolBarIconSize = 24;

static void refreshWidgetFont(QWidget *widget, const QFont &font)
{
    if (widget == nullptr)
    {
        return;
    }

    widget->setFont(font);
    if (auto *widgetStyle = widget->style(); widgetStyle != nullptr)
    {
        widgetStyle->unpolish(widget);
        widgetStyle->polish(widget);
    }

    const auto childWidgets = widget->findChildren<QWidget *>();
    for (QWidget *childWidget : childWidgets)
    {
        childWidget->setFont(font);
        if (auto *childStyle = childWidget->style(); childStyle != nullptr)
        {
            childStyle->unpolish(childWidget);
            childStyle->polish(childWidget);
        }
        childWidget->updateGeometry();
        childWidget->update();
    }

    widget->updateGeometry();
    widget->update();
}
}

IIViewer::IIViewer(QString& needOpenFilePath, QWidget *parent) // NOLINT(readability-function-cognitive-complexity)
    : QMainWindow(parent),
    originSize{QSize{0, 0}, QSize{0, 0}}, 
    openedFile{QString(), QString()},
    openedFileLastModifiedTime{QDateTime(), QDateTime()},
    lastFileWatcherNotifyTime{QDateTime(), QDateTime()},
    lastFileWatcherNotifyPath{QString(), QString()},
    lastFileWatcherNotifyIsWaitProcess{false, false}
{
#ifdef Q_OS_WINDOWS
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
#endif
    if (settings.loadSettingsFromFile())
    {
        workPath = settings.workPath;
    }
    else
    {
        auto pathList = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        if (!pathList.empty())
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

    QApplication::setFont(resolveUiDisplayFont(settings));

    setAcceptDrops(true);
    ui.setupUi(this);
#ifdef Q_OS_WINDOWS
    winId();
    if (isWindows11OrGreater())
    {
        const HWND hwnd = reinterpret_cast<HWND>(winId());
        constexpr int DWMWA_WINDOW_TREATMENT_POLICY = 20;
        int snapValue = 2;
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_TREATMENT_POLICY, &snapValue, sizeof(snapValue));
    }
#endif
    ui.imageLabel.at(LEFT_IMG_WIDGET)->appSettings = &settings;
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->appSettings = &settings;

    const QList<QScreen *> screenInfoList = QApplication::screens();
    bool prevScreenExist = false;
    for(auto *scre: screenInfoList)
    {
        if(scre->name() == settings.windowScreenName)
        {
            prevScreenExist = true;
            break;
        }
    }
    if(!prevScreenExist)
    {
        const QRect screenRect = screenInfoList.at(0)->geometry();
        setGeometry(screenRect.width() / 6, screenRect.height() / 6, screenRect.width() * 2 / 3, screenRect.height() * 2 / 3); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    }
    else
    {
        // Restore window geometry from saved settings
        QRect geometry = settings.windowGeometry;
        
        // Ensure window is visible on current screen
        const QScreen *screen = QApplication::screenAt(geometry.center());
        if (screen != nullptr) {
            const QRect screenRect = screen->geometry();
            geometry = geometry.intersected(screenRect);
        }
        
        if (settings.windowMaximized) {
            showMaximized();
        } else {
            setGeometry(geometry);
        }
    }
    setTitle();

    masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
    // set theme idx by settings
    for(int t_idx=0; t_idx < ui.themes.size(); t_idx++)
    {
        ui.themes.at(t_idx)->setChecked(t_idx == theme_idx);
    }

    connect(ui.openFileLeftAction, &QAction::triggered, this, &IIViewer::onOpenFileAction);
    connect(ui.openFileRightAction, &QAction::triggered, this, &IIViewer::onOpenFileAction);
    connect(ui.reloadFileLeftAction, &QAction::triggered, this, &IIViewer::onReloadFileAction);
    connect(ui.reloadFileRightAction, &QAction::triggered, this, &IIViewer::onReloadFileAction);
    connect(ui.exchangeAreaPreviewBtn, &QPushButton::pressed, this, &IIViewer::exchangeRight2LeftImg);
    connect(ui.exchangeAreaPreviewBtn, &QPushButton::released, this, &IIViewer::restoreLeftImg);
    connect(ui.imageInfoBtn, &QPushButton::clicked, this, &IIViewer::showImageInfo);
    connect(ui.imageDiffInfoBtn, &QPushButton::clicked, this, &IIViewer::showImageDiffReport);
    connect(ui.closeLeftAction, &QAction::triggered, this, &IIViewer::onCloseLeftFileAction);
    connect(ui.closeRightAction, &QAction::triggered, this, &IIViewer::onCloseRightFileAction);
    connect(ui.exitAction, &QAction::triggered, this, [this]()
            { this->close(); });
    // connect(ui.aboutQtAction, &QAction::triggered, []()
    //         { qApp->aboutQt(); });
    connect(ui.aboutThisAction, &QAction::triggered, this, [this]()
            { AboutDlg dlg(this); dlg.exec(); });
    connect(ui.checkUpdateAction, &QAction::triggered, this, &IIViewer::checkUpdate);

    connect(ui.penColorSetBtn, &QPushButton::clicked, this, &IIViewer::selectPenPaintColor);
    connect(ui.penWidthSbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &IIViewer::setPenWidth);
    for (auto &theme : ui.themes)
    {
        connect(theme, &QAction::triggered, this, &IIViewer::setTheme);
    }

    connect(ui.useMoveToolAction, &QAction::triggered, this, &IIViewer::onUseMoveAction);
    connect(ui.useRoiToolAction, &QAction::triggered, this, &IIViewer::onUseRoiAction);
    connect(ui.sysOptionAction, &QAction::triggered, this, &IIViewer::onSysOptionAction);
    connect(ui.workAreaSingleModeAction, &QAction::triggered, this, &IIViewer::onSingleImgModeAction);
    connect(ui.workAreaDoubleModeAction, &QAction::triggered, this, &IIViewer::onDoubleImgModeAction);

    connect(ui.paintOk0, &QPushButton::clicked, this, &IIViewer::handleInputPaintPos0);
    connect(ui.paintOk1, &QPushButton::clicked, this, &IIViewer::handleInputPaintPos1);
    connect(ui.syncRight, &QPushButton::clicked, this, &IIViewer::syncRightPos);
    connect(ui.syncLeft, &QPushButton::clicked, this, &IIViewer::syncLeftPos);
    connect(ui.clearPaintBtn, &QPushButton::clicked, this, &IIViewer::clearPaint);

    connect(ui.plot_rgb_contourf_line, &QPushButton::clicked, this, &IIViewer::plotRgbContourf);
    connect(ui.plot_rgb_hist, &QPushButton::clicked, this, &IIViewer::plotRgbHist);
    connect(ui.imageLabel.at(LEFT_IMG_WIDGET), &ImageWidget::inform_real_Pos, this, &IIViewer::flushPaintPosEdit0);
    connect(ui.imageLabel.at(RIGHT_IMG_WIDGET), &ImageWidget::inform_real_Pos, this, &IIViewer::flushPaintPosEdit1);
    connect(ui.imageLabel.at(LEFT_IMG_WIDGET), &ImageWidget::inform_drag_img, this, &IIViewer::handleRightMouseBtnDrag0);
    connect(ui.imageLabel.at(RIGHT_IMG_WIDGET), &ImageWidget::inform_drag_img, this, &IIViewer::handleRightMouseBtnDrag1);
    connect(ui.imageLabel.at(LEFT_IMG_WIDGET), &ImageWidget::inform_zoom_in, this, &IIViewer::zoomIn1);
    connect(ui.imageLabel.at(LEFT_IMG_WIDGET), &ImageWidget::inform_zoom_out, this, &IIViewer::zoomOut1);
    connect(ui.imageLabel.at(RIGHT_IMG_WIDGET), &ImageWidget::inform_zoom_in, this, &IIViewer::zoomIn0);
    connect(ui.imageLabel.at(RIGHT_IMG_WIDGET), &ImageWidget::inform_zoom_out, this, &IIViewer::zoomOut0);
    connect(ui.imageLabel.at(LEFT_IMG_WIDGET), &ImageWidget::inform_change_master, this, [this]()
        {
            this->masterScrollarea = this->ui.scrollArea.at(LEFT_IMG_WIDGET);
        });
    connect(ui.imageLabel.at(RIGHT_IMG_WIDGET), &ImageWidget::inform_change_master, this, [this]()
        {
            this->masterScrollarea = this->ui.scrollArea.at(RIGHT_IMG_WIDGET);
        });
    connect(ui.imageLabel.at(LEFT_IMG_WIDGET), &ImageWidget::inform_open_file_selector, this, &IIViewer::onOpenFileAction);
    connect(ui.imageLabel.at(RIGHT_IMG_WIDGET), &ImageWidget::inform_open_file_selector, this, &IIViewer::onOpenFileAction);

    connect(ui.scrollArea.at(LEFT_IMG_WIDGET)->horizontalScrollBar(), &QScrollBar::valueChanged, this, &IIViewer::syncScrollArea1_horScBarVal);
    connect(ui.scrollArea.at(LEFT_IMG_WIDGET)->verticalScrollBar(), &QScrollBar::valueChanged, this, &IIViewer::syncScrollArea1_verScBarVal);
    connect(ui.scrollArea.at(LEFT_IMG_WIDGET)->horizontalScrollBar(), &QScrollBar::sliderPressed, this, &IIViewer::setScrollArea0Master);
    connect(ui.scrollArea.at(LEFT_IMG_WIDGET)->verticalScrollBar(), &QScrollBar::sliderPressed, this, &IIViewer::setScrollArea0Master);
    connect(ui.scrollArea.at(RIGHT_IMG_WIDGET)->horizontalScrollBar(), &QScrollBar::valueChanged, this, &IIViewer::syncScrollArea0_horScBarVal);
    connect(ui.scrollArea.at(RIGHT_IMG_WIDGET)->horizontalScrollBar(), &QScrollBar::valueChanged, this, &IIViewer::syncScrollArea0_horScBarVal);
    connect(ui.scrollArea.at(RIGHT_IMG_WIDGET)->verticalScrollBar(), &QScrollBar::valueChanged, this, &IIViewer::syncScrollArea0_verScBarVal);
    connect(ui.scrollArea.at(RIGHT_IMG_WIDGET)->horizontalScrollBar(), &QScrollBar::sliderPressed, this, &IIViewer::setScrollArea1Master);
    connect(ui.scrollArea.at(RIGHT_IMG_WIDGET)->verticalScrollBar(), &QScrollBar::sliderPressed, this, &IIViewer::setScrollArea1Master);

    connect(ui.dataAnalyseAction, &QAction::toggled, this, &IIViewer::toggleDataAnalyseDockWgt);
    connect(ui.playListAction, &QAction::toggled, this, &IIViewer::togglePlayListDockWgt);

    // Connect file history table hover event to show tooltip with file path
    connect(ui.fileHistoryTable, &QTableWidget::cellEntered, this, [this](int row, int column) {
        if (column == 0)
        {
            const QTableWidgetItem *item = ui.fileHistoryTable->item(row, 0);
            if (item)
            {
                const QString filePath = item->data(Qt::UserRole).toString();
                if (!filePath.isEmpty())
                {
                    QToolTip::showText(QCursor::pos(), filePath, ui.fileHistoryTable);
                }
            }
        }
    });

    connect(this, &IIViewer::updateExchangeBtnStatus, this, &IIViewer::updateExchangeBtn);
    connect(this, &IIViewer::updateZoomLabelStatus, this, &IIViewer::updateZoomLabelText);
    connect(this, &IIViewer::needOpenFileFromCmdArgv, this, &IIViewer::openGivenFileFromCmdArgv);

    connect(&openedFileWatcher, &QFileSystemWatcher::fileChanged, this, &IIViewer::openedFileChanged);


    ui.dataAnalyseDockWgt->installEventFilter(this);
    ui.playListDockWgt->installEventFilter(this);
    ui.mainWidget->installEventFilter(this);

    ui.dataAnalyseDockWgt->hide();
    ui.playListDockWgt->hide();
    ui.dataAnalyseAction->setChecked(false);
    ui.playListAction->setChecked(false);
    ui.exchangeAreaPreviewBtn->setEnabled(false);
    ui.imageInfoBtn->setEnabled(false);
    ui.imageDiffInfoBtn->setEnabled(false);
    setWindowIcon(QIcon(":/image/src/resource/aboutlog.png"));
    if (ui.titleBar != nullptr)
    {
        ui.titleBar->setWindowIconPixmap(windowIcon());
        ui.titleBar->updateMaximizeButton(isMaximized());
    }

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

void IIViewer::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && ui.titleBar != nullptr)
    {
        ui.titleBar->updateMaximizeButton(isMaximized());
    }

#ifdef Q_OS_WINDOWS
    if (event->type() == QEvent::WindowStateChange)
    {
    updateNativeShadow(reinterpret_cast<HWND>(winId()), isMaximized());
    }
#endif

    QMainWindow::changeEvent(event);
}

void IIViewer::applyDpiScale(QScreen *screen)
{
    if (screen == nullptr)
    {
        return;
    }

#if QT_VERSION_MAJOR >= 6
    const qreal scale = 1.0;
#else
    const qreal scale = screen->logicalDotsPerInch() / 96.0;
#endif
    const int toolBarHeight = qRound(static_cast<qreal>(baseToolBarHeight) * scale);
    const int toolBarIconSize = qRound(static_cast<qreal>(baseToolBarIconSize) * scale);
    ui.toolBar->setFixedHeight(toolBarHeight);
    ui.toolBar->setIconSize(QSize(toolBarIconSize, toolBarIconSize));
    if (ui.titleBar != nullptr)
    {
        ui.titleBar->updateScale(scale);
        ui.titleBar->updateGeometry();
    }
    if (menuWidget() != nullptr)
    {
        menuWidget()->updateGeometry();
    }
    layout()->invalidate();

    // qDebug() << "[DPI] screen=" << screen->name()
    //          << "dpi=" << screen->logicalDotsPerInch()
    //          << "scale=" << scale
    //          << "titleBarHeight=" << (ui.titleBar != nullptr ? ui.titleBar->height() : -1)
    //          << "toolBarHeight=" << ui.toolBar->height()
    //          << "toolBarIconSize=" << ui.toolBar->iconSize();
}

void IIViewer::ensureWindowHandleConnections()
{
    if (windowHandleConnectionsInitialized)
    {
        return;
    }

    const QWindow *handle = window()->windowHandle();
    if (handle == nullptr)
    {
        return;
    }

    connect(handle, &QWindow::screenChanged, this, [this](QScreen *screen) {
        // qDebug() << "[DPI] screenChanged" << (screen != nullptr ? screen->name() : QStringLiteral("<null>"));
        applyDpiScale(screen);
    });

    if (handle->screen() != nullptr)
    {
        connect(handle->screen(), &QScreen::logicalDotsPerInchChanged, this, [this](qreal dpi) {
            Q_UNUSED(dpi);
            // qDebug() << "[DPI] logicalDotsPerInchChanged" << dpi;
            applyDpiScale(window()->windowHandle() != nullptr ? window()->windowHandle()->screen() : nullptr);
        });
    }

    windowHandleConnectionsInitialized = true;
}


#ifdef Q_OS_WINDOWS
#if QT_VERSION_MAJOR < 6
bool IIViewer::nativeEvent(const QByteArray &eventType, void *message, long *result)
#else
bool IIViewer::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#endif
{
    Q_UNUSED(eventType);
    auto *msg = static_cast<MSG *>(message);

    if (msg == nullptr)
    {
        return false;
    }

    auto setNcButtonHover = [](QPushButton *btn, bool hover) {
        if (btn == nullptr)
            return;
        btn->setProperty("ncHover", hover);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
        btn->update();
    };

    auto clearNcHover = [this, &setNcButtonHover]() {
        if (ncHoveredButton != nullptr)
        {
            setNcButtonHover(ncHoveredButton, false);
            ncHoveredButton = nullptr;
        }
    };

    if (msg->message == WM_NCHITTEST)
    {
        const LONG xPos = GET_X_LPARAM(msg->lParam);
        const LONG yPos = GET_Y_LPARAM(msg->lParam);
#if QT_VERSION_MAJOR >= 6
        const UINT dpi = GetDpiForWindow(msg->hwnd);
        const qreal dpr = dpi / 96.0;
        const QPoint globalPos(qRound(xPos / dpr), qRound(yPos / dpr));
#else
        const QPoint globalPos(xPos, yPos);
#endif
        const QPoint titleBarPos = ui.titleBar != nullptr ? ui.titleBar->mapFromGlobal(globalPos) : QPoint();
        const bool inTitleBar = ui.titleBar != nullptr && ui.titleBar->rect().contains(titleBarPos);
        QWidget *hitWidget = inTitleBar ? ui.titleBar->childAt(titleBarPos) : nullptr;

        if (hitWidget != nullptr)
        {
            const auto objectName = hitWidget->objectName();
            int htResult = 0;
            if (objectName == QLatin1String("titleBarMaximizeButton"))
                htResult = HTMAXBUTTON;
            else if (objectName == QLatin1String("titleBarMinimizeButton"))
                htResult = HTMINBUTTON;
            else if (objectName == QLatin1String("titleBarCloseButton"))
                htResult = HTCLOSE;

            if (htResult != 0)
            {
                *result = htResult;
                return true;
            }
        }

        RECT windowRect{};
        GetWindowRect(msg->hwnd, &windowRect);
        const int resizeBorderWidth = frameBorderWidth(msg->hwnd);
        const int resizeBorderHeight = frameBorderHeight(msg->hwnd);

        const bool onLeft = xPos >= windowRect.left && xPos < windowRect.left + resizeBorderWidth;
        const bool onRight = xPos < windowRect.right && xPos >= windowRect.right - resizeBorderWidth;
        const bool onTop = yPos >= windowRect.top && yPos < windowRect.top + resizeBorderHeight;
        const bool onBottom = yPos < windowRect.bottom && yPos >= windowRect.bottom - resizeBorderHeight;

        if (!isMaximized())
        {
            if (onTop && onLeft)  { *result = HTTOPLEFT;    return true; }
            if (onTop && onRight) { *result = HTTOPRIGHT;   return true; }
            if (onBottom && onLeft)  { *result = HTBOTTOMLEFT;  return true; }
            if (onBottom && onRight) { *result = HTBOTTOMRIGHT; return true; }
            if (onLeft)   { *result = HTLEFT;   return true; }
            if (onRight)  { *result = HTRIGHT;  return true; }
            if (onTop)    { *result = HTTOP;    return true; }
            if (onBottom) { *result = HTBOTTOM; return true; }
        }

        if (ui.titleBar == nullptr)
        {
            return QMainWindow::nativeEvent(eventType, message, result);
        }

        if (ui.titleBar->rect().contains(titleBarPos))
        {
            const bool hitInteractiveWidget = hitWidget != nullptr &&
                (qobject_cast<QPushButton *>(hitWidget) != nullptr ||
                 qobject_cast<QMenuBar *>(hitWidget) != nullptr);
            if (!hitInteractiveWidget)
            {
                *result = HTCAPTION;
                return true;
            }
        }
    }

    if (msg->message == WM_NCMOUSEMOVE)
    {
        if (msg->wParam == HTMAXBUTTON)
        {
            QPushButton *btn = captionButtonFromHitTest(HTMAXBUTTON, ui.titleBar);
            if (btn != ncHoveredButton)
            {
                if (ncHoveredButton != nullptr)
                    setNcButtonHover(ncHoveredButton, false);
                if (btn != nullptr)
                    setNcButtonHover(btn, true);
                ncHoveredButton = btn;
            }
            *result = 0;
            return true;
        }
        QPushButton *btn = captionButtonFromHitTest(msg->wParam, ui.titleBar);
        if (btn != ncHoveredButton)
        {
            if (ncHoveredButton != nullptr)
                setNcButtonHover(ncHoveredButton, false);
            if (btn != nullptr)
                setNcButtonHover(btn, true);
            ncHoveredButton = btn;
        }
        *result = 0;
        return true;
    }
    if (msg->message == WM_NCMOUSEHOVER)
    {
        if (msg->wParam == HTMAXBUTTON)
        {
            *result = 0;
            return true;
        }
        *result = 0;
        return true;
    }
    if (msg->message == WM_NCMOUSELEAVE)
    {
        clearNcHover();
        *result = 0;
        return true;
    }
    if ((msg->message == WM_NCLBUTTONDOWN || msg->message == WM_NCLBUTTONDBLCLK) &&
         isCaptionButtonHitTest(msg->wParam))
    {
        *result = 0;
        return true;
    }
    if (msg->message == WM_NCLBUTTONUP && isCaptionButtonHitTest(msg->wParam))
    {
        WPARAM sc = 0;
        switch (msg->wParam)
        {
        case HTMAXBUTTON: sc = IsZoomed(msg->hwnd) ? SC_RESTORE : SC_MAXIMIZE; break;
        case HTMINBUTTON: sc = SC_MINIMIZE; break;
        case HTCLOSE:     sc = SC_CLOSE; break;
        default: break;
        }
        if (sc != 0)
        {
            SendMessage(msg->hwnd, WM_SYSCOMMAND, sc, 0);
        }
        *result = 0;
        return true;
    }
    if (msg->message == WM_MOUSEMOVE)
    {
        clearNcHover();
    }

    if (msg->message == WM_NCCALCSIZE)
    {
        if (msg->wParam != 0)
        {
            static bool firstNcCalc = true;
            if (firstNcCalc)
            {
                firstNcCalc = false;
                *result = 0;
                return false;
            }
            auto *params = reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);
            const int borderWidth = frameBorderWidth(msg->hwnd);
            const int borderHeight = frameBorderHeight(msg->hwnd);
            const bool maximized = IsZoomed(msg->hwnd) != FALSE;
            const bool keepVisualFrame = maximized || isWindows11OrGreater();
            int topInset = 0;
            if (maximized)
            {
                topInset = borderHeight;
            }
            else if (keepVisualFrame)
            {
                topInset = 1;
            }
            params->rgrc[0].left += keepVisualFrame ? borderWidth : 0;
            params->rgrc[0].top += topInset;
            params->rgrc[0].right -= keepVisualFrame ? borderWidth : 0;
            params->rgrc[0].bottom -= keepVisualFrame ? borderHeight : 0;
            *result = 0;
            return true;
        }
    }

    if (msg->message == WM_GETMINMAXINFO)
    {
        auto *minMaxInfo = reinterpret_cast<MINMAXINFO *>(msg->lParam);
        const HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        if (monitor != nullptr)
        {
            MONITORINFO monitorInfo{};
            monitorInfo.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &monitorInfo);
            const RECT workArea = monitorInfo.rcWork;
            const RECT monitorArea = monitorInfo.rcMonitor;
            const int borderWidth = frameBorderWidth(msg->hwnd);
            const int borderHeight = frameBorderHeight(msg->hwnd);

            minMaxInfo->ptMaxPosition.x = workArea.left - monitorArea.left - borderWidth;
            minMaxInfo->ptMaxPosition.y = workArea.top - monitorArea.top - borderHeight;
            minMaxInfo->ptMaxSize.x = workArea.right - workArea.left + (borderWidth * 2);
            minMaxInfo->ptMaxSize.y = workArea.bottom - workArea.top + (borderHeight * 2);
            minMaxInfo->ptMaxTrackSize = minMaxInfo->ptMaxSize;
            *result = 0;
            return true;
        }
    }

    if (msg->message == WM_DPICHANGED)
    {
        // qDebug() << "[DPI] WM_DPICHANGED" << HIWORD(msg->wParam);
        applyDpiScale(window()->windowHandle() != nullptr ? window()->windowHandle()->screen() : nullptr);
    }

    if (msg->message == WM_WINDOWPOSCHANGED)
    {
        auto *windowPos = reinterpret_cast<WINDOWPOS *>(msg->lParam);
        if (windowPos != nullptr && (windowPos->flags & SWP_FRAMECHANGED))
        {
            updateNativeShadow(msg->hwnd, IsZoomed(msg->hwnd) == TRUE);
        }
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void IIViewer::showEvent(QShowEvent *event)
{
#ifdef Q_OS_WINDOWS
    static bool nativeFrameInitialized = false;
    if (!nativeFrameInitialized)
    {
        nativeFrameInitialized = true;
        const HWND hwnd = reinterpret_cast<HWND>(winId());

        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    updateNativeShadow(reinterpret_cast<HWND>(winId()), isMaximized());
#endif

    ensureWindowHandleConnections();

    if (ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap == nullptr)
    {
        const int scrollWidth = ui.scrollArea.at(LEFT_IMG_WIDGET)->geometry().width();
        const int scrollHeight = ui.scrollArea.at(LEFT_IMG_WIDGET)->geometry().height();
        const int scrollBarWidth = ui.scrollArea.at(LEFT_IMG_WIDGET)->verticalScrollBar()->width();
        const int scrollBarHeight = ui.scrollArea.at(LEFT_IMG_WIDGET)->horizontalScrollBar()->height();
        ui.imageLabelContianer.at(LEFT_IMG_WIDGET)->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
        ui.imageLabelContianer.at(LEFT_IMG_WIDGET)->layout()->setContentsMargins(0, 0, 0, 0);
    }
    if (ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap == nullptr)
    {
        const int scrollWidth = ui.scrollArea.at(RIGHT_IMG_WIDGET)->geometry().width();
        const int scrollHeight = ui.scrollArea.at(RIGHT_IMG_WIDGET)->geometry().height();
        const int scrollBarWidth = ui.scrollArea.at(RIGHT_IMG_WIDGET)->verticalScrollBar()->width();
        const int scrollBarHeight = ui.scrollArea.at(RIGHT_IMG_WIDGET)->horizontalScrollBar()->height();
        ui.imageLabelContianer.at(RIGHT_IMG_WIDGET)->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
        ui.imageLabelContianer.at(RIGHT_IMG_WIDGET)->layout()->setContentsMargins(0, 0, 0, 0);
    }

    QScreen* screen = window()->windowHandle()->screen(); 
    if (screen == nullptr) 
    {
        return;
    }
    applyDpiScale(screen);

    QMainWindow::showEvent(event);
}

void IIViewer::onUseRoiAction(bool check)
{
    if (check)
    {
        ui.useMoveToolAction->setChecked(false);
        ui.imageLabel.at(LEFT_IMG_WIDGET)->setMouseActionPaintRoi();
        ui.imageLabel.at(RIGHT_IMG_WIDGET)->setMouseActionPaintRoi();

        QPixmap penColorBtnIcon(32, 32); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        penColorBtnIcon.fill(settings.penColor);
        ui.penColorSetBtn->setIcon(penColorBtnIcon);

        ui.penColorSetAction->setVisible(true);
        ui.penWidthAction->setVisible(true);
        // ui.toolBar->update();
    }
    else
    {
        ui.imageLabel.at(LEFT_IMG_WIDGET)->setMouseActionNone();
        ui.imageLabel.at(RIGHT_IMG_WIDGET)->setMouseActionNone();

        ui.penColorSetAction->setVisible(false);
        ui.penWidthAction->setVisible(false);
        // ui.toolBar->update();
    }
}

void IIViewer::onUseMoveAction(bool check)
{
    if (check)
    {
        ui.useRoiToolAction->setChecked(false);
        ui.penColorSetAction->setVisible(false);
        ui.penWidthAction->setVisible(false);
        ui.imageLabel.at(LEFT_IMG_WIDGET)->setMouseActionDragImg();
        ui.imageLabel.at(RIGHT_IMG_WIDGET)->setMouseActionDragImg();
    }
    else
    {
        ui.imageLabel.at(LEFT_IMG_WIDGET)->setMouseActionNone();
        ui.imageLabel.at(RIGHT_IMG_WIDGET)->setMouseActionNone();
    }
}

void IIViewer::onSingleImgModeAction(bool check)
{
    ui.workAreaDoubleModeAction->setChecked(!check);
    settings.workAreaDoubleImgMode = !check;
    if(settings.workAreaDoubleImgMode)
    {
        ui.scrollArea.at(1)->show();
        ui.scrollAreaCenterFrame->show();
        ui.openFileRightAction->setEnabled(true);
        ui.reloadFileRightAction->setEnabled(true);
        ui.closeRightAction->setEnabled(true);

        ui.syncLeft->setEnabled(true);
        ui.syncRight->setEnabled(true);

        ui.start_x_edit1->setEnabled(true);
        ui.start_y_edit1->setEnabled(true);
        ui.end_x_edit1->setEnabled(true);
        ui.end_y_edit1->setEnabled(true);
        ui.paintOk1->setEnabled(true);

        for (int i = 0; i < ui.fileHistoryTable->rowCount(); ++i)
        {
            auto* wgt = qobject_cast<QWidget*>(ui.fileHistoryTable->cellWidget(i, 1));
            if (wgt != nullptr)
            {
                for(const auto& childObj : wgt->children())
                {
                    auto* btn = qobject_cast<QPushButton*>(childObj);
                    if(btn != nullptr && btn->text() == "R")
                    {
                        btn->setEnabled(true);
                    }
                }   
            }
        }
    }
    else
    {
        ui.scrollArea.at(1)->hide();
        ui.scrollAreaCenterFrame->hide();
        ui.openFileRightAction->setEnabled(false);
        ui.reloadFileRightAction->setEnabled(false);
        ui.closeRightAction->setEnabled(false);

        ui.syncLeft->setEnabled(false);
        ui.syncRight->setEnabled(false);

        ui.start_x_edit1->setEnabled(false);
        ui.start_y_edit1->setEnabled(false);
        ui.end_x_edit1->setEnabled(false);
        ui.end_y_edit1->setEnabled(false);
        ui.paintOk1->setEnabled(false);

        for (int i = 0; i < ui.fileHistoryTable->rowCount(); ++i)
        {
            auto* wgt = qobject_cast<QWidget*>(ui.fileHistoryTable->cellWidget(i, 1));
            if (wgt != nullptr)
            {
                for(const auto& childObj : wgt->children())
                {
                    auto* btn = qobject_cast<QPushButton*>(childObj);
                    if(btn != nullptr && btn->text() == "R")
                    {
                        btn->setEnabled(false);
                    }
                }   
            }
        }
    }
    ui.mainWidget->adjustSize();
}

void IIViewer::onDoubleImgModeAction(bool check)
{
    ui.workAreaSingleModeAction->setChecked(!check);
    settings.workAreaDoubleImgMode = check;
    if(settings.workAreaDoubleImgMode)
    {
        ui.scrollArea.at(1)->show();
        ui.scrollAreaCenterFrame->show();
        ui.openFileRightAction->setEnabled(true);
        ui.reloadFileRightAction->setEnabled(true);
        ui.closeRightAction->setEnabled(true);

        ui.syncLeft->setEnabled(true);
        ui.syncRight->setEnabled(true);

        ui.start_x_edit1->setEnabled(true);
        ui.start_y_edit1->setEnabled(true);
        ui.end_x_edit1->setEnabled(true);
        ui.end_y_edit1->setEnabled(true);
        ui.paintOk1->setEnabled(true);

        for (int i = 0; i < ui.fileHistoryTable->rowCount(); ++i)
        {
            auto* wgt = qobject_cast<QWidget*>(ui.fileHistoryTable->cellWidget(i, 1));
            if (wgt != nullptr)
            {
                for(const auto& childObj : wgt->children())
                {
                    auto* btn = qobject_cast<QPushButton*>(childObj);
                    if(btn != nullptr && btn->text() == "R")
                    {
                        btn->setEnabled(true);
                    }
                }   
            }
        }
    }
    else
    {
        ui.scrollArea.at(1)->hide();
        ui.scrollAreaCenterFrame->hide();
        ui.openFileRightAction->setEnabled(false);
        ui.reloadFileRightAction->setEnabled(false);
        ui.closeRightAction->setEnabled(false);

        ui.syncLeft->setEnabled(false);
        ui.syncRight->setEnabled(false);

        ui.start_x_edit1->setEnabled(false);
        ui.start_y_edit1->setEnabled(false);
        ui.end_x_edit1->setEnabled(false);
        ui.end_y_edit1->setEnabled(false);
        ui.paintOk1->setEnabled(false);

        for (int i = 0; i < ui.fileHistoryTable->rowCount(); ++i)
        {
            auto* wgt = qobject_cast<QWidget*>(ui.fileHistoryTable->cellWidget(i, 1));
            if (wgt != nullptr)
            {
                for(const auto& childObj : wgt->children())
                {
                    auto* btn = qobject_cast<QPushButton*>(childObj);
                    if(btn != nullptr && btn->text() == "R")
                    {
                        btn->setEnabled(false);
                    }
                }   
            }
        }
    }
    ui.mainWidget->adjustSize();

    if(check && ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap != nullptr && ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap != nullptr)
    {
        if(ui.scrollArea.at(RIGHT_IMG_WIDGET)->horizontalScrollBar() != nullptr)
        {
            const int hval = ui.scrollArea.at(RIGHT_IMG_WIDGET)->horizontalScrollBar()->value();
            syncScrollArea0_horScBarVal(hval);
        }
        if(ui.scrollArea.at(RIGHT_IMG_WIDGET)->verticalScrollBar() != nullptr) 
        {
            const int vval = ui.scrollArea.at(RIGHT_IMG_WIDGET)->verticalScrollBar()->value();
            syncScrollArea0_verScBarVal(vval);
        }
    }
}

void IIViewer::onSysOptionAction(bool check)
{
    Q_UNUSED(check);
    IIVOptionDialog dlg(this);
    dlg.set_uv_disp_mode(this->settings.uv_value_disp_mode);
    dlg.set_pix_val_bg_color_index(this->settings.pix_val_bg_index);
    dlg.set_pix_val_custom_bg_color(this->settings.pix_val_cus_bg_color);
    dlg.set_ui_display_font(resolveUiDisplayFont(this->settings));
    if(dlg.exec() == IIVOptionDialog::DialogCode::Accepted)
    {
        this->settings.uv_value_disp_mode = dlg.uv_disp_mode;
        this->settings.pix_val_bg_index = dlg.pix_val_bg_color_index;
        this->settings.pix_val_cus_bg_color = dlg.pix_val_cus_bg_color;
        this->settings.uiFontFamily = dlg.ui_font_family;
        this->settings.uiFontPointSize = dlg.ui_font_point_size;
        const QFont uiFont = resolveUiDisplayFont(this->settings);
        QApplication::setFont(uiFont);

        refreshWidgetFont(this, uiFont);
        refreshWidgetFont(ui.titleBar, uiFont);
        refreshWidgetFont(ui.dataAnalyseDockWgt, uiFont);
        refreshWidgetFont(ui.playListDockWgt, uiFont);
        refreshWidgetFont(ui.dataAnalyseDockWgt != nullptr ? ui.dataAnalyseDockWgt->widget() : nullptr, uiFont);
        refreshWidgetFont(ui.playListDockWgt != nullptr ? ui.playListDockWgt->widget() : nullptr, uiFont);

        if (ui.titleBar != nullptr)
        {
            if (ui.titleBar->menuBar() != nullptr)
            {
                refreshWidgetFont(ui.titleBar->menuBar(), uiFont);
            }
        }
    }
}

void IIViewer::checkUpdate()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished, [this, manager](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray const response = reply->readAll();
            QJsonDocument const jsonDoc = QJsonDocument::fromJson(response);
            const QJsonObject jsonObj = jsonDoc.object();
            QString latestVersion = jsonObj.value("tag_name").toString();
            if (latestVersion.startsWith('v', Qt::CaseSensitivity::CaseInsensitive))
            {
                latestVersion.remove(QChar('v'), Qt::CaseSensitivity::CaseInsensitive);
             }
             const QVersionNumber currentVersion{IIViewer_VERSION_MAJOR, IIViewer_VERSION_MINOR, IIViewer_VERSION_PATCH}; // 当前版本号 
             const QVersionNumber newVersion = QVersionNumber::fromString(latestVersion);
            //  qDebug() << currentVersion << newVersion << latestVersion;
             if (newVersion > currentVersion) {
                const QString text("<a href=\"https://github.com/JonahZeng/IIViewer/releases\">Click here to github release page download new version</a>");
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
    const QUrl url("https://api.github.com/repos/JonahZeng/IIViewer/releases/latest");
    const QNetworkRequest request(url); 
    manager->get(request);
}

void IIViewer::openedFileChanged(const QString &filePath)
{
    if(filePath == openedFile.at(LEFT_IMG_WIDGET))
    {
        const QDateTime curNotiyTime = QDateTime::currentDateTime();
        if(filePath == lastFileWatcherNotifyPath.at(0) && (lastFileWatcherNotifyTime.at(0).msecsTo(curNotiyTime) < 100 || lastFileWatcherNotifyIsWaitProcess.at(0))) // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        {
            return;
        }

        lastFileWatcherNotifyPath.at(0) = filePath;
        lastFileWatcherNotifyTime.at(0) = curNotiyTime;

        auto curModifiedTime = QFileInfo(filePath).lastModified();
        if (curModifiedTime > openedFileLastModifiedTime.at(LEFT_IMG_WIDGET))
        {
            lastFileWatcherNotifyIsWaitProcess.at(0) = true;
            auto resp = QMessageBox::information(this, tr("file changed"), filePath + tr(" has been changed, reload it?"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::No);
            if (resp == QMessageBox::StandardButton::Ok)
            {
                reLoadFile(LEFT_IMG_WIDGET);
                masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
                openedFileLastModifiedTime.at(LEFT_IMG_WIDGET) = curModifiedTime;
            }
            lastFileWatcherNotifyIsWaitProcess.at(0) = false;
        }
    }
    else if(filePath == openedFile.at(RIGHT_IMG_WIDGET))
    {
        const QDateTime curNotiyTime = QDateTime::currentDateTime();
        if(filePath == lastFileWatcherNotifyPath.at(1) && (lastFileWatcherNotifyTime.at(1).msecsTo(curNotiyTime) < 100 || lastFileWatcherNotifyIsWaitProcess.at(1))) // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        {
            return;
        }

        lastFileWatcherNotifyPath.at(1) = filePath;
        lastFileWatcherNotifyTime.at(1) = curNotiyTime;

        auto curModifiedTime = QFileInfo(filePath).lastModified();
        if (curModifiedTime > openedFileLastModifiedTime.at(RIGHT_IMG_WIDGET))
        {
            lastFileWatcherNotifyIsWaitProcess.at(1) = true;
            auto resp = QMessageBox::information(this, tr("file changed"), filePath + tr(" has been changed, reload it?"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::No);
            if (resp == QMessageBox::StandardButton::Ok)
            {
                reLoadFile(RIGHT_IMG_WIDGET);
                masterScrollarea = ui.scrollArea.at(RIGHT_IMG_WIDGET);
                openedFileLastModifiedTime.at(RIGHT_IMG_WIDGET) = curModifiedTime;
            }
            lastFileWatcherNotifyIsWaitProcess.at(1) = false;
        }
    }
    else
    {
        openedFileWatcher.removePath(filePath);
    }
}

void IIViewer::closeEvent(QCloseEvent *event)
{
    auto reply = QMessageBox::question(this, tr("Confirm"), tr("Are you sure to quit?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
    if (reply == QMessageBox::Yes)
    {
        // Save window position and screen info
        const QRect geometry = this->geometry();
        const QPoint center = geometry.center();
        const QScreen *screen = QApplication::screenAt(center);

        if (screen != nullptr)
        {
            settings.windowGeometry = geometry;
            settings.windowScreenName = screen->name();

            // Verify the screen still exists (multi-screen environment may change)
            bool screenExists = false;
            for (const QScreen *screen_it : QApplication::screens())
            {
                if (screen_it->name() == screen->name())
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

        // Save window maximized state
        settings.windowMaximized = isMaximized();

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

void IIViewer::setTitle()
{
#ifdef Q_OS_WINDOWS
#else
    QString openedFile0_t(openedFile[0]);
    QString openedFile1_t(openedFile[1]);
    if (openedFile[0].length() > 50) // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    {
        openedFile0_t = openedFile[0].left(20) + ".........." + openedFile[0].right(20); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    }
    if (openedFile[1].length() > 50) // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    {
        openedFile1_t = openedFile[1].left(20) + ".........." + openedFile[1].right(20); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
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
#endif
}

void IIViewer::addFileToHistory(const QString &filePath)
{
    QFileInfo const fileInfo(filePath);
    QString const fileName = fileInfo.fileName();

    // Check if file already exists in history
    bool fileExists = false;
    int existingRow = -1;
    for (int i = 0; i < ui.fileHistoryTable->rowCount(); ++i)
    {
        QTableWidgetItem const *item = ui.fileHistoryTable->item(i, 0);
        if (item != nullptr && item->data(Qt::UserRole).toString() == filePath)
        {
            fileExists = true;
            existingRow = i;
            break;
        }
    }
    
    if (fileExists)
    {
        // Move existing file to top
        ui.fileHistoryTable->insertRow(0);
        QTableWidgetItem *newItem = new QTableWidgetItem(fileName);
        newItem->setData(Qt::UserRole, filePath);
        ui.fileHistoryTable->setItem(0, 0, newItem);

        // Create button widget for second column
        QWidget *buttonWidget = new QWidget();
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
        buttonLayout->setContentsMargins(2, 2, 2, 2);
        buttonLayout->setSpacing(2);
        
        QPushButton *leftBtn = new QPushButton("L", buttonWidget);
        leftBtn->setMaximumWidth(25); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        leftBtn->setToolTip(QCoreApplication::translate("mainWindow", "Open in left", nullptr));
        connect(leftBtn, &QPushButton::clicked, this, [this, filePath]() {
            QFileInfo const fileInfo(filePath);
            if (fileInfo.isFile()) {
                onCloseLeftFileAction();
                loadFile(filePath, LEFT_IMG_WIDGET);
                masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
                openedFileWatcher.addPath(filePath);
                openedFileLastModifiedTime.at(LEFT_IMG_WIDGET) = QFileInfo(filePath).lastModified();
                setTitle();
                emit updateExchangeBtnStatus();
                emit updateZoomLabelStatus();
            } });

        QPushButton *rightBtn = new QPushButton("R", buttonWidget);
        rightBtn->setMaximumWidth(25); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        rightBtn->setToolTip(QCoreApplication::translate("mainWindow", "Open in right", nullptr));
        connect(rightBtn, &QPushButton::clicked, this, [this, filePath]() {
            QFileInfo const fileInfo(filePath);
            if (fileInfo.isFile()) {
                onCloseRightFileAction();
                loadFile(filePath, RIGHT_IMG_WIDGET);
                masterScrollarea = ui.scrollArea.at(RIGHT_IMG_WIDGET);
                openedFileWatcher.addPath(filePath);
                openedFileLastModifiedTime.at(RIGHT_IMG_WIDGET) = QFileInfo(filePath).lastModified();
                setTitle();
                emit updateExchangeBtnStatus();
                emit updateZoomLabelStatus();
            }
        });
        
        buttonLayout->addStretch();
        buttonLayout->addWidget(leftBtn);
        buttonLayout->addWidget(rightBtn);
        
        ui.fileHistoryTable->setCellWidget(0, 1, buttonWidget);

        ui.fileHistoryTable->removeRow(existingRow + 1);
    }
    else
    {
        // Add new file to top
        ui.fileHistoryTable->insertRow(0);
        QTableWidgetItem *newItem = new QTableWidgetItem(fileName);
        newItem->setData(Qt::UserRole, filePath);
        ui.fileHistoryTable->setItem(0, 0, newItem);
    
        // Create button widget for second column
        QWidget *buttonWidget = new QWidget();
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
        buttonLayout->setContentsMargins(2, 2, 2, 2);
        buttonLayout->setSpacing(2);
        
        QPushButton *leftBtn = new QPushButton("L", buttonWidget);
        leftBtn->setMaximumWidth(25); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        leftBtn->setToolTip(QCoreApplication::translate("mainWindow", "Open in left", nullptr));
        connect(leftBtn, &QPushButton::clicked, this, [this, filePath]() {
            QFileInfo const fileInfo(filePath);
            if (fileInfo.isFile()) {
                onCloseLeftFileAction();
                loadFile(filePath, LEFT_IMG_WIDGET);
                masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
                openedFileWatcher.addPath(filePath);
                openedFileLastModifiedTime.at(LEFT_IMG_WIDGET) = QFileInfo(filePath).lastModified();
                setTitle();
                emit updateExchangeBtnStatus();
                emit updateZoomLabelStatus();
            }
        });
        
        QPushButton *rightBtn = new QPushButton("R", buttonWidget);
        rightBtn->setMaximumWidth(25); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        rightBtn->setToolTip(QCoreApplication::translate("mainWindow", "Open in right", nullptr));
        connect(rightBtn, &QPushButton::clicked, this, [this, filePath]() {
            const QFileInfo fileInfo(filePath);
            if (fileInfo.isFile()) {
                onCloseRightFileAction();
                loadFile(filePath, RIGHT_IMG_WIDGET);
                masterScrollarea = ui.scrollArea.at(RIGHT_IMG_WIDGET);
                openedFileWatcher.addPath(filePath);
                openedFileLastModifiedTime.at(RIGHT_IMG_WIDGET) = QFileInfo(filePath).lastModified();
                setTitle();
                emit updateExchangeBtnStatus();
                emit updateZoomLabelStatus();
            }
        });
        
        buttonLayout->addStretch();
        buttonLayout->addWidget(leftBtn);
        buttonLayout->addWidget(rightBtn);
        
        ui.fileHistoryTable->setCellWidget(0, 1, buttonWidget);
       
        // Remove oldest file if exceeds 50 entries
        if (ui.fileHistoryTable->rowCount() > 50) // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        {
            ui.fileHistoryTable->removeRow(50); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        }
    }
}

void IIViewer::onOpenFileAction()
{
    // QString path = settings.workPath;
    auto fileName = QFileDialog::getOpenFileName(this, tr("open images"), settings.workPath,
        "Images files(*.jpg *.JPG *.jpeg *.JPEG *.png *.PNG *.bmp *.BMP *.tif *.TIF *.tiff *.TIFF *.heic *.HEIC);;Raw files(*.raw *.RAW);;Pnm files(*.pnm *.PNM);;Pgm files(*.pgm *.PGM);;yuv files(*.yuv *.YUV);;Tiff files(*.tif *.TIF *.tiff *.TIFF);;All files(*.*)");

    if (fileName.isEmpty())
    {
        return;
    }

    const QFileInfo info(fileName);
    settings.workPath = info.absolutePath();
    if (sender() == static_cast<QObject *>(ui.openFileLeftAction) || sender() == static_cast<QObject*>(ui.imageLabel.at(LEFT_IMG_WIDGET)))
    {
        // onCloseLeftFileAction(); // 这里删除关闭文件监控
        loadFile(fileName, LEFT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
        openedFileWatcher.addPath(fileName); // 添加新文件监控
        openedFileLastModifiedTime.at(LEFT_IMG_WIDGET) = QFileInfo(fileName).lastModified(); // 记录新文件的修改时间
    }
    else if (sender() == static_cast<QObject *>(ui.openFileRightAction) || sender() == static_cast<QObject*>(ui.imageLabel.at(RIGHT_IMG_WIDGET)))
    {
        // onCloseRightFileAction();
        loadFile(fileName, RIGHT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea.at(RIGHT_IMG_WIDGET);
        openedFileWatcher.addPath(fileName); // 添加新文件监控
        openedFileLastModifiedTime.at(RIGHT_IMG_WIDGET) = QFileInfo(fileName).lastModified(); // 记录新文件的修改时间
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
void IIViewer::openGivenFileFromCmdArgv(QString& image)
{
    const QFileInfo info(image);
    // qDebug() << info.suffix();
    const QString suf = info.suffix().toLower();
    if(suf == "jpg" || suf == "png" || suf == "bmp" || suf == "tif" || suf == "tiff" || suf == "raw" || suf == "yuv" || suf == "pnm" || suf == "pgm" || suf == "heic")
    {
        onCloseLeftFileAction();
        loadFile(image, LEFT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
        openedFileWatcher.addPath(image);
        openedFileLastModifiedTime.at(LEFT_IMG_WIDGET) = QFileInfo(image).lastModified();
        setTitle();
        emit updateExchangeBtnStatus();
        emit updateZoomLabelStatus();
    }
}

void IIViewer::loadFilePostProcessLayoutAndScrollValue(int leftOrRight)
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

    if (!openedFile.at(slave).isEmpty())
    {
        const int zoomIdx = ui.imageLabel.at(slave)->zoomIdx;
        if (zoomIdx > 2)
        {
            ui.imageLabel.at(master)->zoomIn(zoomIdx);
        }
        else if (zoomIdx < 2)
        {
            ui.imageLabel.at(master)->zoomOut(zoomIdx);
        }
        ui.imageLabelContianer.at(master)->layout()->setContentsMargins(ui.imageLabelContianer.at(slave)->layout()->contentsMargins());
        ui.imageLabelContianer.at(master)->resize(ui.imageLabelContianer.at(slave)->size());

        int value = ui.scrollArea.at(slave)->horizontalScrollBar()->value();
        ui.scrollArea.at(master)->horizontalScrollBar()->setValue(value);
        value = ui.scrollArea.at(slave)->verticalScrollBar()->value();
        ui.scrollArea.at(master)->verticalScrollBar()->setValue(value);
    }
    else
    {
        const int scrollAreaClientWidth = ui.scrollArea.at(master)->width() - ui.scrollArea.at(master)->verticalScrollBar()->width();
        const int scrollAreaClientHeight = ui.scrollArea.at(master)->height() - ui.scrollArea.at(master)->horizontalScrollBar()->height();

        ui.imageLabelContianer.at(master)->resize(std::max(scrollAreaClientWidth, originSize.at(master).width()), std::max(scrollAreaClientHeight, originSize.at(master).height()));
        int margin_hor = 0;
        int margin_ver = 0;
        if (scrollAreaClientWidth > originSize.at(master).width())
        {
            margin_hor = (scrollAreaClientWidth - originSize.at(master).width()) / 2;
        }
        if (scrollAreaClientHeight > originSize.at(master).height())
        {
            margin_ver = (scrollAreaClientHeight - originSize.at(master).height()) / 2;
        }
        ui.imageLabelContianer.at(master)->layout()->setContentsMargins(margin_hor, margin_ver, margin_hor, margin_ver);

        const int hmax = ui.scrollArea.at(master)->horizontalScrollBar()->maximum();
        const int hmin = ui.scrollArea.at(master)->horizontalScrollBar()->minimum();
        const int vmax = ui.scrollArea.at(master)->verticalScrollBar()->maximum();
        const int vmin = ui.scrollArea.at(master)->verticalScrollBar()->minimum();

        ui.scrollArea.at(master)->horizontalScrollBar()->setValue((hmax - hmin) / 2);
        ui.scrollArea.at(master)->verticalScrollBar()->setValue((vmax - vmin) / 2);
    }
}

void IIViewer::reLoadFile(int scrollArea) // NOLINT(readability-function-cognitive-complexity)
{
    QString& dstFn = openedFile.at(scrollArea == LEFT_IMG_WIDGET ? 0 : 1);
    if (dstFn.endsWith(".jpg", Qt::CaseInsensitive) || dstFn.endsWith(".jpeg", Qt::CaseInsensitive) || dstFn.endsWith(".png", Qt::CaseInsensitive) || dstFn.endsWith(".bmp", Qt::CaseInsensitive))
    {
        QImageReader reader(dstFn);
        reader.setAutoTransform(true);
        if (!reader.canRead())
        {
            const QString txt_info = QString("can not open ") + dstFn + QString(" as image!");
            QMessageBox::information(this, tr("error"), txt_info, QMessageBox::StandardButton::Ok);
            return;
        }

        if (scrollArea == LEFT_IMG_WIDGET)
        {
            if (!openedFile.at(1).isEmpty())
            {
                if (reader.size() != originSize.at(1))
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            // openedFile[0] = dstFn;
            originSize.at(0) = reader.size();
            setImage(dstFn, LEFT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针

            // loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
        }
        else if (scrollArea == RIGHT_IMG_WIDGET)
        {
            if (openedFile.at(0).length() > 0)
            {
                if (reader.size() != originSize.at(0))
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            // openedFile[1] = dstFn;
            originSize.at(1) = reader.size();
            // ui.imageLabelContianer[1]->resize(originSize[1]);
            setImage(dstFn, RIGHT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针

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
    else if (dstFn.endsWith(".heic", Qt::CaseInsensitive))
    {
        loadHeifFile(dstFn, scrollArea, true);
    }
    else
    {
        QMessageBox::warning(this, tr("not support"), tr("this format file not support yet!"), QMessageBox::StandardButton::Ok);
        return;
    }
}

void IIViewer::loadFile(const QString &fileName, int scrollArea) // NOLINT(readability-function-cognitive-complexity)
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
            const QString txt_info = QString("can not open ") + fileName + QString(" as image!");
            QMessageBox::information(this, tr("error"), txt_info, QMessageBox::StandardButton::Ok);
            return;
        }

        if (scrollArea == LEFT_IMG_WIDGET)
        {
            if (!openedFile.at(1).isEmpty())
            {
                if (reader.size() != originSize.at(1))
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            onCloseLeftFileAction();
            openedFile.at(0) = fileName;
            originSize.at(0) = reader.size();
            setImage(openedFile.at(0), LEFT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针

            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
        else if (scrollArea == RIGHT_IMG_WIDGET)
        {
            if (openedFile.at(0).length() > 0)
            {
                if (reader.size() != originSize.at(0))
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            onCloseRightFileAction();
            openedFile.at(1) = fileName;
            originSize.at(1) = reader.size();
            // ui.imageLabelContianer[1]->resize(originSize[1]);
            setImage(openedFile.at(1), RIGHT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针

            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (fileName.endsWith(".heic", Qt::CaseInsensitive))
    {
        loadHeifFile(fileName, scrollArea);
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

void IIViewer::loadYuvFile(const QString &fileName, int scrollArea, bool reload) // NOLINT(readability-function-cognitive-complexity)
{
    YuvFileInfoDlg dlg(this);

    auto yuvtp = settings.yuvType;
    if(yuvtp >= YuvType::YUV444_INTERLEAVE && yuvtp <= YuvType::YUV422_UYVY_AS1)
    {
        dlg.ui.formatComboBox->setCurrentIndex(static_cast<int>(yuvtp));
    }
    dlg.ui.bitDepthSpinBox->setValue(settings.yuv_bitDepth);
    dlg.ui.widthLineEdit->setText(QString::asprintf("%d", settings.yuv_width));
    dlg.ui.heightLineEdit->setText(QString::asprintf("%d", settings.yuv_height));

    if (dlg.exec() != YuvFileInfoDlg::Accepted)
    {
        return;
    }

    const int bitDepth = dlg.ui.bitDepthSpinBox->value();
    int pixSize = 2;
    if (bitDepth <= BIT8)
    {
        pixSize = 1;
    }
    else if (bitDepth > BIT8 && bitDepth <= BIT16)
    {
        pixSize = 2;
    }
    else
    {
        QMessageBox::warning(this, tr("error"), tr("yuv bits > 16"), QMessageBox::StandardButton::Ok);
        return;
    }

    const int width = dlg.ui.widthLineEdit->text().toInt();
    const int height = dlg.ui.heightLineEdit->text().toInt();
    qint64 totalSize = 0;
    YuvType yuv_tp = YuvType::YUV444_INTERLEAVE;
    const int curIdx = dlg.ui.formatComboBox->currentIndex();
    if (curIdx == YuvType::YUV444_INTERLEAVE)
    {
        yuv_tp = YuvType::YUV444_INTERLEAVE;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U;
    }
    else if (curIdx == YuvType::YUV444_PLANAR)
    {
        yuv_tp = YuvType::YUV444_PLANAR;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U;
    }
    else if (curIdx == YuvType::YUV422_UYVY)
    {
        yuv_tp = YuvType::YUV422_UYVY;
        totalSize = static_cast<qint64>(width) * height * pixSize * 2U;
    }
    else if (curIdx == YuvType::YUV422_YUYV)
    {
        yuv_tp = YuvType::YUV422_YUYV;
        totalSize = static_cast<qint64>(width) * height * pixSize * 2U;
    }
    else if (curIdx == YuvType::YUV420_NV12)
    {
        yuv_tp = YuvType::YUV420_NV12;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U / 2U;
    }
    else if (curIdx == YuvType::YUV420_NV21)
    {
        yuv_tp = YuvType::YUV420_NV21;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U / 2U;
    }
    else if (curIdx == YuvType::YUV420P_YU12)
    {
        yuv_tp = YuvType::YUV420P_YU12;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U / 2U;
    }
    else if (curIdx == YuvType::YUV420P_YV12)
    {
        yuv_tp = YuvType::YUV420P_YV12;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U / 2U;
    }
    else if (curIdx == YuvType::YUV400)
    {
        yuv_tp = YuvType::YUV400;
        totalSize = static_cast<qint64>(width) * height * pixSize;
    }
    else if (curIdx == YuvType::YUV422_YUYV_AS1)
    {
        yuv_tp = YuvType::YUV422_YUYV_AS1;
        totalSize = static_cast<qint64>(width) * height * pixSize * 2U;
    }
    else if (curIdx == YuvType::YUV422_UYVY_AS1)
    {
        yuv_tp = YuvType::YUV422_UYVY_AS1;
        totalSize = static_cast<qint64>(width) * height * pixSize * 2U;
    }

    settings.yuvType = yuv_tp;
    settings.yuv_bitDepth = bitDepth;
    settings.yuv_width = width;
    settings.yuv_height = height;

    const QFileInfo fileInfo(fileName);
    const qint64 yuvFileSize = fileInfo.size();
    if (totalSize > yuvFileSize)
    {
        QMessageBox::warning(this, tr("error"), tr("yuv file size < your require"), QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (QSize(width, height) != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        openedFile.at(0) = fileName;
        originSize.at(0) = QSize(width, height);
        setYuvImage(openedFile.at(0), yuv_tp, bitDepth, width, height, pixSize, LEFT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (!openedFile.at(0).isEmpty())
        {
            if (QSize(width, height) != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseRightFileAction();
        }
        openedFile.at(1) = fileName;
        originSize.at(1) = QSize(width, height);
        setYuvImage(openedFile.at(1), yuv_tp, bitDepth, width, height, pixSize, RIGHT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadRawFile(const QString &fileName, int scrollArea, bool reload) // NOLINT(readability-function-cognitive-complexity)
{
    RawFileInfoDlg dlg(this);
    switch (settings.rawByType)
    {
    case BayerPatternType::RGGB:
        dlg.ui.RGGBRadioButton->setChecked(true);
        break;
    case BayerPatternType::GRBG:
        dlg.ui.GRBGRadioButton->setChecked(true);
        break;
    case BayerPatternType::GBRG:
        dlg.ui.GBRGRadioButton->setChecked(true);
        break;
    case BayerPatternType::BGGR:
        dlg.ui.BGGRRadioButton->setChecked(true);
        break;
    case BayerPatternType::RGGIR:
        dlg.ui.RGGIRRadioButton->setChecked(true);
        break;
    case BayerPatternType::BGGIR:
        dlg.ui.BGGIRRadioButton->setChecked(true);
        break;
    case BayerPatternType::GRIRG:
        dlg.ui.GRIRGRadioButton->setChecked(true);
        break;
    case BayerPatternType::GBIRG:
        dlg.ui.GBIRGRadioButton->setChecked(true);
        break;
    case BayerPatternType::GIRRG:
        dlg.ui.GIRRGRadioButton->setChecked(true);
        break;
    case BayerPatternType::GIRBG:
        dlg.ui.GIRBGRadioButton->setChecked(true);
        break;
    case BayerPatternType::IRGGR:
        dlg.ui.IRGGRRadioButton->setChecked(true);
        break;
    case BayerPatternType::IRGGB:
        dlg.ui.IRGGBRadioButton->setChecked(true);
        break;
    case BayerPatternType::MONO:
        dlg.ui.MONORadioButton->setChecked(true);
        break;
    
    default:
        break;
    }
    
    auto byteOrder = settings.rawByteOrder;
    if (byteOrder == ByteOrderType::RAW_LITTLE_ENDIAN)
    {
        dlg.ui.little_endian->setChecked(true);
    }
    else if (byteOrder == ByteOrderType::RAW_BIG_ENDIAN)
    {
        dlg.ui.big_endian->setChecked(true);
    }
    if(!settings.raw_compact)
    {
        dlg.ui.BitDepthComboBox->setCurrentText(QString::asprintf("%d", settings.raw_bitDepth));
    }
    else
    {
        dlg.ui.BitDepthComboBox->setCurrentText(QString::asprintf("%d-comp", settings.raw_bitDepth));
    }

    dlg.ui.WidthLineEdit->setText(QString::asprintf("%d", settings.raw_width));
    dlg.ui.HeightLineEdit->setText(QString::asprintf("%d", settings.raw_height));

    if (dlg.exec() != RawFileInfoDlg::Accepted)
    {
        return;
    }
    BayerPatternType bay = BayerPatternType::BAYER_UNKNOW;
    if (dlg.ui.RGGBRadioButton->isChecked())
    {
        bay = BayerPatternType::RGGB;
    }
    else if (dlg.ui.GRBGRadioButton->isChecked())
    {
        bay = BayerPatternType::GRBG;
    }
    else if (dlg.ui.GBRGRadioButton->isChecked())
    {
        bay = BayerPatternType::GBRG;
    }
    else if (dlg.ui.BGGRRadioButton->isChecked())
    {
        bay = BayerPatternType::BGGR;
    }
    else if (dlg.ui.RGGIRRadioButton->isChecked())
    {
        bay = BayerPatternType::RGGIR;
    }
    else if (dlg.ui.BGGIRRadioButton->isChecked())
    {
        bay = BayerPatternType::BGGIR;
    }
    else if (dlg.ui.GRIRGRadioButton->isChecked())
    {
        bay = BayerPatternType::GRIRG;
    }
    else if (dlg.ui.GBIRGRadioButton->isChecked())
    {
        bay = BayerPatternType::GBIRG;
    }
    else if (dlg.ui.GIRRGRadioButton->isChecked())
    {
        bay = BayerPatternType::GIRRG;
    }
    else if (dlg.ui.GIRBGRadioButton->isChecked())
    {
        bay = BayerPatternType::GIRBG;
    }
    else if (dlg.ui.IRGGRRadioButton->isChecked())
    {
        bay = BayerPatternType::IRGGR;
    }
    else if (dlg.ui.IRGGBRadioButton->isChecked())
    {
        bay = BayerPatternType::IRGGB;
    }
    else if (dlg.ui.MONORadioButton->isChecked())
    {
        bay = BayerPatternType::MONO;
    }

    auto order = ByteOrderType::RAW_LITTLE_ENDIAN;
    if (dlg.ui.big_endian->isChecked())
    {
        order = ByteOrderType::RAW_BIG_ENDIAN;
    }
    bool isInt = false;
    const QString bit_option_str = dlg.ui.BitDepthComboBox->currentText();
    int bitDepth = bit_option_str.toInt(&isInt, 10); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    bool raw_compact = false;
    if(!isInt && bit_option_str.endsWith(QString("-comp"))) // 12-comp, sep by '-'
    {
        auto bit_opt_list = bit_option_str.split(QChar('-'));
        bitDepth = bit_opt_list.at(0).toInt();
        raw_compact = true;
    }
    const int width = dlg.ui.WidthLineEdit->text().toInt();
    const int height = dlg.ui.HeightLineEdit->text().toInt();

    settings.rawByType = bay;
    settings.raw_bitDepth = bitDepth;
    settings.raw_width = width;
    settings.raw_height = height;
    settings.rawByteOrder = order;
    settings.raw_compact = raw_compact;

    int pixSize = 2;
    if (bitDepth <= BIT8)
    {
        pixSize = 1;
    }
    else if (bitDepth > BIT8 && bitDepth <= BIT16)
    {
        pixSize = 2;
    }
    else if (bitDepth > BIT16 && bitDepth <= BIT32)
    {
        pixSize = 4;
    }

    const QFileInfo fileInfo(fileName);
    const qint64 rawFileSize = fileInfo.size();
    if(raw_compact)
    {
        if(width * height * bitDepth / BIT8 > rawFileSize)
        {
            QMessageBox::information(this, tr("error"), tr("raw file size < your input"), QMessageBox::StandardButton::Ok);
            return;
        }
    }
    else if (static_cast<qint64>(pixSize) * width * height > rawFileSize)
    {
        QMessageBox::information(this, tr("error"), tr("raw file size < your input"), QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (QSize(width, height) != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        openedFile.at(0) = fileName;
        originSize.at(0) = QSize(width, height);
        setRawImage(openedFile.at(0), bay, order, bitDepth, raw_compact, width, height, LEFT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET); 
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (!openedFile.at(0).isEmpty())
        {
            if (QSize(width, height) != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseRightFileAction();
        }
        openedFile.at(1) = fileName;
        originSize.at(1) = QSize(width, height);
        setRawImage(openedFile.at(1), bay, order, bitDepth, raw_compact, width, height, RIGHT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadPnmFile(const QString &fileName, int scrollArea, bool reload)
{
    const QImageReader reader(fileName);
    if (!reader.canRead())
    {
        const QString txt_info = QString("can not open ") + fileName + QString(" as image!");
        QMessageBox::information(this, tr("error"), txt_info, QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (reader.size() != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        openedFile.at(0) = fileName;
        originSize.at(0) = reader.size();
        setImage(openedFile.at(0), LEFT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (openedFile.at(0).length() > 0)
        {
            if (reader.size() != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseRightFileAction();
        }
        openedFile.at(1) = fileName;
        originSize.at(1) = reader.size();
        setImage(openedFile.at(1), RIGHT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadPgmFile(const QString &fileName, int scrollArea, bool reload)
{
    const QImageReader reader(fileName);
    if (!reader.canRead())
    {
        const QString txt_info = QString("can not open ") + fileName + QString(" as image!");
        QMessageBox::information(this, tr("error"), txt_info, QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (reader.size() != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        openedFile.at(0) = fileName;
        originSize.at(0) = reader.size();
        setImage(openedFile.at(0), LEFT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (openedFile.at(0).length() > 0)
        {
            if (reader.size() != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseRightFileAction();
        }
        openedFile.at(1) = fileName;
        originSize.at(1) = reader.size();
        setImage(openedFile.at(1), RIGHT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadHeifFile(const QString &fileName, int scrollArea, bool reload)
{
    // Load HEIC image using libheif
    heif_context* ctx = heif_context_alloc();
    if (ctx != nullptr)
    {
        const QString t_info = QString("Failed to allocate heif context for ") + fileName;
        QMessageBox::information(this, tr("error"), t_info, QMessageBox::StandardButton::Ok);
        return;
    }

    heif_error error = heif_context_read_from_file(ctx, fileName.toUtf8().constData(), nullptr);
    if (error.code != heif_error_Ok)
    {
        const QString t_info = QString("Failed to read HEIC file: ") + fileName + QString("\nError: ") + error.message;
        QMessageBox::information(this, tr("error"), t_info, QMessageBox::StandardButton::Ok);
        heif_context_free(ctx);
        return;
    }

    heif_image_handle* handle = nullptr; // NOLINT(misc-const-correctness)
    error = heif_context_get_primary_image_handle(ctx, &handle);
    if (error.code != heif_error_Ok)
    {
        const QString t_info = QString("Failed to get primary image handle from ") + fileName;
        QMessageBox::information(this, tr("error"), t_info, QMessageBox::StandardButton::Ok);
        heif_context_free(ctx);
        return;
    }

    const int width = heif_image_handle_get_width(handle);
    const int height = heif_image_handle_get_height(handle);

    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (QSize(width, height) != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                heif_image_handle_release(handle);
                heif_context_free(ctx);
                return;
            }
        }
    }
    else if(scrollArea == RIGHT_IMG_WIDGET)
    {
        if (openedFile.at(0).length() > 0)
        {
            if (QSize(width, height) != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                heif_image_handle_release(handle);
                heif_context_free(ctx);
                return;
            }
        }
    }

    const int l_bit_depth = heif_image_handle_get_luma_bits_per_pixel(handle);
    const int c_bit_depth = heif_image_handle_get_chroma_bits_per_pixel(handle);
    heif_colorspace csp = heif_colorspace::heif_colorspace_undefined;
    heif_chroma cch = heif_chroma::heif_chroma_undefined;
    heif_image_handle_get_preferred_decoding_colorspace(handle, &csp, &cch);
    if (l_bit_depth > BIT8 || c_bit_depth > BIT8 || csp != heif_colorspace::heif_colorspace_YCbCr || \
        (cch != heif_chroma::heif_chroma_420 && cch != heif_chroma::heif_chroma_422 && cch != heif_chroma::heif_chroma_444))
    {
        const QString txt_info = QString("Failed to decode HEIC image: ") + fileName + QString("\nError: ") + QString("not 8bit yuv image");
        QMessageBox::information(this, tr("error"), txt_info, QMessageBox::StandardButton::Ok);
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return;
    }

    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        openedFile.at(0) = fileName;
        originSize.at(0) = QSize(width, height);
        setImage(openedFile.at(0), LEFT_IMG_WIDGET);
        if(!reload)
        { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseRightFileAction();
        }
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        openedFile.at(1) = fileName;
        originSize.at(1) = QSize(width, height);
        setImage(openedFile.at(1), RIGHT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::setImage(QString &imageName, int leftOrRight)
{
    ui.imageLabel.at(leftOrRight)->paintBegin = false;
    ui.imageLabel.at(leftOrRight)->paintEnd = false;
    ui.imageLabel.at(leftOrRight)->imgName = &imageName;
    ui.imageLabel.at(leftOrRight)->setPixmap();
    ui.imageLabel.at(leftOrRight)->lastCurve = std::array<QPoint, IMG_PREVIEW_ADJ_CURVE_DOTS>{QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0)};
}

void IIViewer::setRawImage(QString &imageName, BayerPatternType bay, ByteOrderType order, int bitDepth, bool compact, int width, int height, int leftOrRight)
{
    ui.imageLabel.at(leftOrRight)->paintBegin = false;
    ui.imageLabel.at(leftOrRight)->paintEnd = false;
    ui.imageLabel.at(leftOrRight)->imgName = &imageName;
    ui.imageLabel.at(leftOrRight)->setPixmap(bay, order, bitDepth, compact, width, height);
    ui.imageLabel.at(leftOrRight)->lastCurve = std::array<QPoint, IMG_PREVIEW_ADJ_CURVE_DOTS>{QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0)};
}

void IIViewer::setYuvImage(QString &imageName, YuvType yuv_tp, int bitDepth, int width, int height, int pixSize, int leftOrRight)
{
    ui.imageLabel.at(leftOrRight)->paintBegin = false;
    ui.imageLabel.at(leftOrRight)->paintEnd = false;
    ui.imageLabel.at(leftOrRight)->imgName = &imageName;
    ui.imageLabel.at(leftOrRight)->setPixmap(yuv_tp, bitDepth, width, height, pixSize);
    ui.imageLabel.at(leftOrRight)->lastCurve = std::array<QPoint, IMG_PREVIEW_ADJ_CURVE_DOTS>{QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0), QPoint(0, 0)};
}

void IIViewer::onCloseLeftFileAction()
{
    if (ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap == nullptr)
    {
        return;
    }
    if(ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap != nullptr)
    {
        masterScrollarea = ui.scrollArea.at(RIGHT_IMG_WIDGET);
    }

    ui.imageLabel.at(LEFT_IMG_WIDGET)->releaseBuffer();
    ui.imageLabel.at(LEFT_IMG_WIDGET)->paintBegin = false;
    ui.imageLabel.at(LEFT_IMG_WIDGET)->paintEnd = false;
    ui.imageLabel.at(LEFT_IMG_WIDGET)->zoomIdx = 2; // 关闭之后恢复到1.0x倍率
    ui.imageLabel.at(LEFT_IMG_WIDGET)->openedImgType = UNKNOW_IMG;
    ui.imageLabel.at(LEFT_IMG_WIDGET)->rawBayerType = BayerPatternType::BAYER_UNKNOW;
    ui.imageLabel.at(LEFT_IMG_WIDGET)->rawDataBit = 0;
    openedFileWatcher.removePath(openedFile.at(0));
    openedFile.at(0).clear();
    setTitle();
    ui.start_x_edit0->clear();
    ui.start_y_edit0->clear();
    ui.end_x_edit0->clear();
    ui.end_y_edit0->clear();

    const int scrollWidth = ui.scrollArea.at(LEFT_IMG_WIDGET)->geometry().width();
    const int scrollHeight = ui.scrollArea.at(LEFT_IMG_WIDGET)->geometry().height();
    const int scrollBarWidth = ui.scrollArea.at(LEFT_IMG_WIDGET)->verticalScrollBar()->width();
    const int scrollBarHeight = ui.scrollArea.at(LEFT_IMG_WIDGET)->horizontalScrollBar()->height();
    ui.imageLabelContianer.at(LEFT_IMG_WIDGET)->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
    ui.imageLabelContianer.at(LEFT_IMG_WIDGET)->layout()->setContentsMargins(0, 0, 0, 0);

    emit updateExchangeBtnStatus();
    emit updateZoomLabelStatus();
}

void IIViewer::onCloseRightFileAction()
{
    if (ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap == nullptr)
    {
        return;
    }
    if(ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap != nullptr)
    {
        masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
    }

    ui.imageLabel.at(RIGHT_IMG_WIDGET)->releaseBuffer();
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->paintBegin = false;
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->paintEnd = false;
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->zoomIdx = 2; // 关闭之后恢复到1.0x倍率
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->openedImgType = UNKNOW_IMG;
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->rawBayerType = BayerPatternType::BAYER_UNKNOW;
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->rawDataBit = 0;
    openedFileWatcher.removePath(openedFile.at(1));
    openedFile.at(1).clear();
    setTitle();
    ui.start_x_edit1->clear();
    ui.start_y_edit1->clear();
    ui.end_x_edit1->clear();
    ui.end_y_edit1->clear();

    const int scrollWidth = ui.scrollArea.at(RIGHT_IMG_WIDGET)->geometry().width();
    const int scrollHeight = ui.scrollArea.at(RIGHT_IMG_WIDGET)->geometry().height();
    const int scrollBarWidth = ui.scrollArea.at(RIGHT_IMG_WIDGET)->verticalScrollBar()->width();
    const int scrollBarHeight = ui.scrollArea.at(RIGHT_IMG_WIDGET)->horizontalScrollBar()->height();
    ui.imageLabelContianer.at(RIGHT_IMG_WIDGET)->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
    ui.imageLabelContianer.at(RIGHT_IMG_WIDGET)->layout()->setContentsMargins(0, 0, 0, 0);

    emit updateExchangeBtnStatus();
    emit updateZoomLabelStatus();
}

void IIViewer::onReloadFileAction()
{
    if (sender() == static_cast<QObject *>(ui.reloadFileLeftAction))
    {
        // loadFile(openedFile[0], LEFT_IMG_WIDGET);
        reLoadFile(LEFT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
        openedFileLastModifiedTime.at(LEFT_IMG_WIDGET) = QFileInfo(openedFile.at(LEFT_IMG_WIDGET)).lastModified();
    }
    else if (sender() == static_cast<QObject *>(ui.reloadFileRightAction))
    {
        // loadFile(openedFile[1], RIGHT_IMG_WIDGET);
        reLoadFile(RIGHT_IMG_WIDGET);
        masterScrollarea = ui.scrollArea.at(RIGHT_IMG_WIDGET);
        openedFileLastModifiedTime.at(RIGHT_IMG_WIDGET) = QFileInfo(openedFile.at(RIGHT_IMG_WIDGET)).lastModified();
    }
    // setTitle();

    emit updateExchangeBtnStatus();
    // emit updateZoomLabelStatus();
}

void IIViewer::setPenWidth(int width)
{
    ui.imageLabel.at(0)->setPenWidth(width);
    ui.imageLabel.at(1)->setPenWidth(width);
}

void IIViewer::setTheme()
{
    int t_idx = 0;
    auto themeKeys = QStyleFactory::keys();
    for (auto &theme : ui.themes)
    {
        if (sender() == static_cast<QObject *>(theme))
        {
            QApplication::setStyle(themeKeys.at(t_idx));
            settings.theme = themeKeys.at(t_idx);
            theme->setChecked(true);
        }
        else
        {
            theme->setChecked(false);
        }
        ++t_idx;
    }
}

void IIViewer::toggleDataAnalyseDockWgt(bool checked)
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

void IIViewer::togglePlayListDockWgt(bool checked)
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

void IIViewer::syncRightPos() const
{
    ui.start_x_edit1->setText(ui.start_x_edit0->text());
    ui.start_y_edit1->setText(ui.start_y_edit0->text());
    ui.end_x_edit1->setText(ui.end_x_edit0->text());
    ui.end_y_edit1->setText(ui.end_y_edit0->text());
}

void IIViewer::syncLeftPos() const
{
    ui.start_x_edit0->setText(ui.start_x_edit1->text());
    ui.start_y_edit0->setText(ui.start_y_edit1->text());
    ui.end_x_edit0->setText(ui.end_x_edit1->text());
    ui.end_y_edit0->setText(ui.end_y_edit1->text());
}

void IIViewer::flushPaintPosEdit0(QPointF startPt, QPointF endPt) const
{
    const QPoint realStartPt = startPt.toPoint();
    const QPoint realEndPt = endPt.toPoint();
    ui.start_x_edit0->setText(QString::asprintf("%d", realStartPt.x()));
    ui.start_y_edit0->setText(QString::asprintf("%d", realStartPt.y()));
    ui.end_x_edit0->setText(QString::asprintf("%d", realEndPt.x()));
    ui.end_y_edit0->setText(QString::asprintf("%d", realEndPt.y()));
}

void IIViewer::flushPaintPosEdit1(QPointF startPt, QPointF endPt) const
{
    const QPoint realStartPt = startPt.toPoint();
    const QPoint realEndPt = endPt.toPoint();
    ui.start_x_edit1->setText(QString::asprintf("%d", realStartPt.x()));
    ui.start_y_edit1->setText(QString::asprintf("%d", realStartPt.y()));
    ui.end_x_edit1->setText(QString::asprintf("%d", realEndPt.x()));
    ui.end_y_edit1->setText(QString::asprintf("%d", realEndPt.y()));
}

void IIViewer::handleInputPaintPos0()
{
    if (ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap == nullptr)
    {
        return;
    }
    if (ui.start_x_edit0->text().length() == 0 || ui.start_y_edit0->text().length() == 0 || ui.end_x_edit0->text().length() == 0 || ui.end_y_edit0->text().length() == 0)
    {
        return;
    }

    const int maxWidth = ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap->width();
    const int maxHeight = ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap->height();
    int start_x = ui.start_x_edit0->text().toInt();
    start_x = start_x >= maxWidth ? maxWidth : start_x;
    int start_y = ui.start_y_edit0->text().toInt();
    start_y = start_y >= maxHeight ? start_x : start_y;
    int end_x = ui.end_x_edit0->text().toInt();
    end_x = end_x >= maxWidth ? maxWidth : end_x;
    int end_y = ui.end_y_edit0->text().toInt();
    end_y = end_y >= maxHeight ? maxHeight : end_y;
    const float scale_ratio = ui.imageLabel.at(0)->zoomList.at(ui.imageLabel.at(0)->zoomIdx);
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.paintCoordinates.at(0) = QPoint(static_cast<int>(static_cast<float>(start_x) * scale_ratio), static_cast<int>(static_cast<float>(start_y) * scale_ratio));
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.paintCoordinates.at(1) = QPoint(static_cast<int>(static_cast<float>(end_x) * scale_ratio), static_cast<int>(static_cast<float>(end_y) * scale_ratio));
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.originPaintCoordinates.at(0) = QPoint(start_x, start_y);
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.originPaintCoordinates.at(1) = QPoint(end_x, end_y);
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.originScaleRatio = 1.0F;
    ui.imageLabel.at(LEFT_IMG_WIDGET)->paintEnd = true;
    ui.imageLabel.at(LEFT_IMG_WIDGET)->repaint();
}

void IIViewer::handleInputPaintPos1()
{
    if (ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap == nullptr)
    {
        return;
    }
    if (ui.start_x_edit1->text().length() == 0 || ui.start_y_edit1->text().length() == 0 || ui.end_x_edit1->text().length() == 0 || ui.end_y_edit1->text().length() == 0)
    {
        return;
    }

    const int maxWidth = ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap->width();
    const int maxHeight = ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap->height();
    int start_x = ui.start_x_edit1->text().toInt();
    start_x = start_x >= maxWidth ? maxWidth : start_x;
    int start_y = ui.start_y_edit1->text().toInt();
    start_y = start_y >= maxHeight ? start_x : start_y;
    int end_x = ui.end_x_edit1->text().toInt();
    end_x = end_x >= maxWidth ? maxWidth : end_x;
    int end_y = ui.end_y_edit1->text().toInt();
    end_y = end_y >= maxHeight ? maxHeight : end_y;
    const float scale_ratio = ui.imageLabel.at(RIGHT_IMG_WIDGET)->zoomList.at(ui.imageLabel.at(RIGHT_IMG_WIDGET)->zoomIdx);
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.paintCoordinates.at(0) = QPoint(static_cast<int>(static_cast<float>(start_x) * scale_ratio), static_cast<int>(static_cast<float>(start_y) * scale_ratio));
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.paintCoordinates.at(1) = QPoint(static_cast<int>(static_cast<float>(end_x) * scale_ratio), static_cast<int>(static_cast<float>(end_y) * scale_ratio));
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.originPaintCoordinates.at(0) = QPoint(start_x, start_y);
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.originPaintCoordinates.at(1) = QPoint(end_x, end_y);
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.originScaleRatio = 1.0F;
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->paintEnd = true;
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->repaint();
}

void IIViewer::plotRgbContourf()
{
    bool leftPrepared = true;
    bool rightPrepared = true;
    if (ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap == nullptr)
    {
        leftPrepared = false;
    }
    if (ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap == nullptr)
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
    const QPoint st0(ui.start_x_edit0->text().toInt(), ui.start_y_edit0->text().toInt());
    const QPoint st1(ui.end_x_edit0->text().toInt(), ui.end_y_edit0->text().toInt());

    DataVisualDialog *dlg = new DataVisualDialog(this, leftPrepared, ui.imageLabel.at(LEFT_IMG_WIDGET), st0, st1);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void IIViewer::plotRgbHist()
{
}

void IIViewer::selectPenPaintColor()
{
    QColorDialog dlg(settings.penColor, this);
    if (dlg.exec() == QColorDialog::DialogCode::Accepted)
    {
        settings.penColor = dlg.selectedColor();
        ui.imageLabel.at(LEFT_IMG_WIDGET)->setPaintPenColor(settings.penColor);
        ui.imageLabel.at(RIGHT_IMG_WIDGET)->setPaintPenColor(settings.penColor);
    }
    QPixmap penColorBtnIcon(32, 32); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    penColorBtnIcon.fill(settings.penColor);
    ui.penColorSetBtn->setIcon(penColorBtnIcon);
}

void IIViewer::zoomIn0(int zoomIdx)
{
    masterScrollarea = ui.scrollArea.at(1);
    if (zoomIdx <= ZOOM_LIST_LENGTH - 1 && zoomIdx >= 0)
    {
        ui.zoomRatioLabel->setText(ui.imageLabel.at(0)->zoomListLabel.at(zoomIdx));
    }
    if (ui.imageLabel.at(0)->pixMap != nullptr)
    {
        ui.imageLabel.at(0)->zoomIn(zoomIdx);
        ui.imageLabelContianer.at(0)->resize(ui.imageLabelContianer.at(1)->size());
        ui.imageLabelContianer.at(0)->layout()->setContentsMargins(ui.imageLabelContianer.at(1)->layout()->contentsMargins());
    }
}

/**
 * @brief 由left widget触发的信号，right widget响应
 * 
 * @param zoomIdx 
 */
void IIViewer::zoomIn1(int zoomIdx)
{
    masterScrollarea = ui.scrollArea.at(0);
    if (zoomIdx <= ZOOM_LIST_LENGTH - 1 && zoomIdx >= 0)
    {
        ui.zoomRatioLabel->setText(ui.imageLabel.at(1)->zoomListLabel.at(zoomIdx));
    }
    if (ui.imageLabel.at(1)->pixMap != nullptr)
    {
        ui.imageLabel.at(1)->zoomIn(zoomIdx);
        ui.imageLabelContianer.at(1)->resize(ui.imageLabelContianer.at(0)->size());
        ui.imageLabelContianer.at(1)->layout()->setContentsMargins(ui.imageLabelContianer.at(0)->layout()->contentsMargins());
    }
}

/**
 * @brief 由right widget触发的信号，left widget响应
 * 
 * @param zoomIdx 
 */
void IIViewer::zoomOut0(int zoomIdx)
{
    masterScrollarea = ui.scrollArea.at(1);
    if (zoomIdx <= ZOOM_LIST_LENGTH - 1 && zoomIdx >= 0)
    {
        ui.zoomRatioLabel->setText(ui.imageLabel.at(0)->zoomListLabel.at(zoomIdx));
    }
    if (ui.imageLabel.at(0)->pixMap != nullptr)
    {
        ui.imageLabel.at(0)->zoomOut(zoomIdx);
        ui.imageLabelContianer.at(0)->resize(ui.imageLabelContianer.at(1)->size());
        ui.imageLabelContianer.at(0)->layout()->setContentsMargins(ui.imageLabelContianer.at(1)->layout()->contentsMargins());
    }
}

void IIViewer::zoomOut1(int zoomIdx)
{
    masterScrollarea = ui.scrollArea.at(0);
    if (zoomIdx <= ZOOM_LIST_LENGTH - 1 && zoomIdx >= 0)
    {
        ui.zoomRatioLabel->setText(ui.imageLabel.at(1)->zoomListLabel.at(zoomIdx));
    }
    if (ui.imageLabel.at(1)->pixMap != nullptr)
    {
        ui.imageLabel.at(1)->zoomOut(zoomIdx);
        ui.imageLabelContianer.at(1)->resize(ui.imageLabelContianer.at(0)->size());
        ui.imageLabelContianer.at(1)->layout()->setContentsMargins(ui.imageLabelContianer.at(0)->layout()->contentsMargins());
    }
}

void IIViewer::clearPaint()
{
    ui.start_x_edit1->clear();
    ui.start_y_edit1->clear();
    ui.end_x_edit1->clear();
    ui.end_y_edit1->clear();
    ui.start_x_edit0->clear();
    ui.start_y_edit0->clear();
    ui.end_x_edit0->clear();
    ui.end_y_edit0->clear();
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.paintCoordinates.at(0) = QPoint();
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.paintCoordinates.at(1) = QPoint();
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.originPaintCoordinates.at(0) = QPoint();
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.originPaintCoordinates.at(1) = QPoint();
    ui.imageLabel.at(LEFT_IMG_WIDGET)->ptCodInfo.originScaleRatio = 1.0F;
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.paintCoordinates.at(0) = QPoint();
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.paintCoordinates.at(1) = QPoint();
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.originPaintCoordinates.at(0) = QPoint();
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.originPaintCoordinates.at(1) = QPoint();
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->ptCodInfo.originScaleRatio = 1.0F;
    ui.imageLabel.at(LEFT_IMG_WIDGET)->repaint();
    ui.imageLabel.at(RIGHT_IMG_WIDGET)->repaint();
}

void IIViewer::handleRightMouseBtnDrag0(QPointF startPt, QPointF endPt)
{
    if (!ui.scrollArea.at(0)->horizontalScrollBar()->isVisible() && !ui.scrollArea.at(0)->verticalScrollBar()->isVisible())
    {
        return;
    }
    masterScrollarea = ui.scrollArea.at(0);
    const int curHscrollVal = ui.scrollArea.at(0)->horizontalScrollBar()->value();
    const int delta_x = static_cast<int>(startPt.x() - endPt.x());
    const int curVscrollVal = ui.scrollArea.at(0)->verticalScrollBar()->value();
    const int delta_y = static_cast<int>(startPt.y() - endPt.y());
    ui.scrollArea.at(0)->horizontalScrollBar()->setValue(curHscrollVal + delta_x);
    ui.scrollArea.at(0)->verticalScrollBar()->setValue(curVscrollVal + delta_y);
}

void IIViewer::handleRightMouseBtnDrag1(QPointF startPt, QPointF endPt)
{
    if (!ui.scrollArea.at(1)->horizontalScrollBar()->isVisible() && !ui.scrollArea.at(1)->verticalScrollBar()->isVisible())
    {
        return;
    }
    masterScrollarea = ui.scrollArea.at(1);
    const int curHscrollVal = ui.scrollArea.at(1)->horizontalScrollBar()->value();
    const int delta_x = static_cast<int>(startPt.x() - endPt.x());
    const int curVscrollVal = ui.scrollArea.at(1)->verticalScrollBar()->value();
    const int delta_y = static_cast<int>(startPt.y() - endPt.y());
    ui.scrollArea.at(1)->horizontalScrollBar()->setValue(curHscrollVal + delta_x);
    ui.scrollArea.at(1)->verticalScrollBar()->setValue(curVscrollVal + delta_y);
}

/**
 * @brief 同步left image widget horizontal bar value
 * 
 * @param value horizontal bar value
 */
void IIViewer::syncScrollArea1_horScBarVal(int value)
{
    ui.imageLabel.at(0)->update(ui.imageLabel.at(0)->zoomTextRect.toRect());
    ui.imageLabel.at(0)->update(ui.imageLabel.at(0)->pixValPaintRect.toRect());
    if (ui.imageLabel.at(1)->pixMap == nullptr)
    {
        return;
    }
    if (masterScrollarea == ui.scrollArea.at(1))
    {
        return;
    }

    ui.imageLabelContianer.at(1)->layout()->setContentsMargins(ui.imageLabelContianer.at(0)->layout()->contentsMargins());
    ui.imageLabelContianer.at(1)->resize(ui.imageLabelContianer.at(0)->size());
    ui.scrollArea.at(1)->horizontalScrollBar()->setValue(value);
}

/**
 * @brief 同步left image widget vertical bar value
 * 
 * @param value vertical bar value
 */
void IIViewer::syncScrollArea1_verScBarVal(int value)
{
    ui.imageLabel.at(0)->update(ui.imageLabel.at(0)->zoomTextRect.toRect());
    ui.imageLabel.at(0)->update(ui.imageLabel.at(0)->pixValPaintRect.toRect());
    if (ui.imageLabel.at(1)->pixMap == nullptr)
    {
        return;
    }
    if (masterScrollarea == ui.scrollArea.at(1))
    {
        return;
    }

    ui.imageLabelContianer.at(1)->layout()->setContentsMargins(ui.imageLabelContianer.at(0)->layout()->contentsMargins());
    ui.imageLabelContianer.at(1)->resize(ui.imageLabelContianer.at(0)->size());
    ui.scrollArea.at(1)->verticalScrollBar()->setValue(value);
}

void IIViewer::syncScrollArea0_horScBarVal(int value)
{
    ui.imageLabel.at(1)->update(ui.imageLabel.at(1)->zoomTextRect.toRect());
    ui.imageLabel.at(1)->update(ui.imageLabel.at(1)->pixValPaintRect.toRect());
    if (ui.imageLabel.at(0)->pixMap == nullptr)
    {
        return;
    }
    if (masterScrollarea == ui.scrollArea.at(0))
    {
        return;
    }

    ui.imageLabelContianer.at(0)->layout()->setContentsMargins(ui.imageLabelContianer.at(1)->layout()->contentsMargins());
    ui.imageLabelContianer.at(0)->resize(ui.imageLabelContianer.at(1)->size());
    ui.scrollArea.at(0)->horizontalScrollBar()->setValue(value);
}

void IIViewer::syncScrollArea0_verScBarVal(int value)
{
    ui.imageLabel.at(1)->update(ui.imageLabel.at(1)->zoomTextRect.toRect());
    ui.imageLabel.at(1)->update(ui.imageLabel.at(1)->pixValPaintRect.toRect());
    if (ui.imageLabel.at(0)->pixMap == nullptr)
    {
        return;
    }
    if (masterScrollarea == ui.scrollArea.at(0))
    {
        return;
    }

    ui.imageLabelContianer.at(0)->layout()->setContentsMargins(ui.imageLabelContianer.at(1)->layout()->contentsMargins());
    ui.imageLabelContianer.at(0)->resize(ui.imageLabelContianer.at(1)->size());
    ui.scrollArea.at(0)->verticalScrollBar()->setValue(value);
}

void IIViewer::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

void IIViewer::dropEvent(QDropEvent *event)
{
    const QRect mainWidget_rect = ui.mainWidget->geometry();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QPointF dropPt = event->posF();
#else
    QPointF dropPt = event->position();
#endif
    if (dropPt.x() < mainWidget_rect.left() || dropPt.x() > mainWidget_rect.right() || dropPt.y() < mainWidget_rect.top() || dropPt.y() > mainWidget_rect.bottom())
    {
        return;
    }
    const QMimeData *mData = event->mimeData();
    if (!mData->hasUrls())
    {
        return;
    }
    auto urlList = mData->urls();
    if (urlList.length() == 1)
    {
        auto scrollArea0_rect = ui.scrollArea.at(LEFT_IMG_WIDGET)->geometry().translated(mainWidget_rect.topLeft());
        auto scrollArea1_rect = ui.scrollArea.at(RIGHT_IMG_WIDGET)->geometry().translated(mainWidget_rect.topLeft());
        if (scrollArea0_rect.left() < dropPt.x() && dropPt.x() < scrollArea0_rect.right())
        {
            auto fileName0 = urlList.at(0).toLocalFile();
            if (fileName0.isEmpty())
            {
                return;
            }
            loadFile(fileName0, LEFT_IMG_WIDGET); // 这里删除关闭文件监控
            setTitle();
            masterScrollarea = ui.scrollArea.at(LEFT_IMG_WIDGET);
            openedFileWatcher.addPath(fileName0); // 添加新文件监控
            openedFileLastModifiedTime.at(LEFT_IMG_WIDGET) = QFileInfo(fileName0).lastModified(); // 记录新文件的修改时间
        }
        else if (scrollArea1_rect.left() < dropPt.x() && dropPt.x() < scrollArea1_rect.right())
        {
            auto fileName0 = urlList.at(0).toLocalFile();
            if (fileName0.isEmpty())
            {
                return;
            }
            loadFile(fileName0, RIGHT_IMG_WIDGET); // 这里删除关闭文件监控
            setTitle();
            masterScrollarea = ui.scrollArea.at(RIGHT_IMG_WIDGET);
            openedFileWatcher.addPath(fileName0); // 添加新文件监控
            openedFileLastModifiedTime.at(RIGHT_IMG_WIDGET) = QFileInfo(fileName0).lastModified(); // 记录新文件的修改时间
        }
        const QFileInfo info(urlList.at(0).toLocalFile());
        settings.workPath = info.absolutePath();
    }
    else
    {
        QMessageBox::critical(this, tr("error"), tr("At most 1 image!"), QMessageBox::StandardButton::Ok);
    }
    emit updateExchangeBtnStatus();
    emit updateZoomLabelStatus();
}

bool IIViewer::eventFilter(QObject *obj, QEvent *event)
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
        if (ui.imageLabel.at(LEFT_IMG_WIDGET)->pixMap == nullptr)
        {
            const int scrollWidth = ui.scrollArea.at(LEFT_IMG_WIDGET)->geometry().width();
            const int scrollHeight = ui.scrollArea.at(LEFT_IMG_WIDGET)->geometry().height();
            const int scrollBarWidth = ui.scrollArea.at(LEFT_IMG_WIDGET)->verticalScrollBar()->width();
            const int scrollBarHeight = ui.scrollArea.at(LEFT_IMG_WIDGET)->horizontalScrollBar()->height();
            ui.imageLabelContianer.at(LEFT_IMG_WIDGET)->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
            ui.imageLabelContianer.at(LEFT_IMG_WIDGET)->layout()->setContentsMargins(0, 0, 0, 0);
        }

        if (ui.imageLabel.at(RIGHT_IMG_WIDGET)->pixMap == nullptr)
        {
            const int scrollWidth = ui.scrollArea.at(RIGHT_IMG_WIDGET)->geometry().width();
            const int scrollHeight = ui.scrollArea.at(RIGHT_IMG_WIDGET)->geometry().height();
            const int scrollBarWidth = ui.scrollArea.at(RIGHT_IMG_WIDGET)->verticalScrollBar()->width();
            const int scrollBarHeight = ui.scrollArea.at(RIGHT_IMG_WIDGET)->horizontalScrollBar()->height();
            ui.imageLabelContianer.at(RIGHT_IMG_WIDGET)->resize(scrollWidth - scrollBarWidth, scrollHeight - scrollBarHeight);
            ui.imageLabelContianer.at(RIGHT_IMG_WIDGET)->layout()->setContentsMargins(0, 0, 0, 0);
        }
        //        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

IIViewer::~IIViewer()
{
    settings.dumpSettingsToFile();
}
