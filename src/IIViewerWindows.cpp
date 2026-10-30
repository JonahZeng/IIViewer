// Windows 平台独有的原生窗口逻辑：无边框窗口、DWM 阴影/边框、Win11 贴靠布局、
// 以及处理 WM_NCHITTEST / WM_NCCALCSIZE 等原生消息。
// 其他平台由本文件末尾的空实现兜底，调用点无需条件编译。

#include "IIViewer.h"
#include "TitleBar.h"

#ifdef Q_OS_WINDOWS

#include <QMenuBar>
#include <QStyle>
#include <QWindow>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

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

void IIViewer::applyNativeWindowFlags()
{
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
}

void IIViewer::applyNativeSnapLayout()
{
    winId();
    if (isWindows11OrGreater())
    {
        const HWND hwnd = reinterpret_cast<HWND>(winId());
        constexpr int DWMWA_WINDOW_TREATMENT_POLICY = 20;
        int snapValue = 2;
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_TREATMENT_POLICY, &snapValue, sizeof(snapValue));
    }
}

void IIViewer::refreshNativeShadow()
{
    updateNativeShadow(reinterpret_cast<HWND>(winId()), isMaximized());
}

void IIViewer::initNativeFrame()
{
    static bool nativeFrameInitialized = false;
    if (!nativeFrameInitialized)
    {
        nativeFrameInitialized = true;
        const HWND hwnd = reinterpret_cast<HWND>(winId());

        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    updateNativeShadow(reinterpret_cast<HWND>(winId()), isMaximized());
}

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
        if (btn == nullptr) {
            return;
        }
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

#else

// 非 Windows 平台没有原生边框处理，保持空实现，调用点无需条件编译
void IIViewer::applyNativeWindowFlags() {}
void IIViewer::applyNativeSnapLayout() {}
void IIViewer::refreshNativeShadow() {}
void IIViewer::initNativeFrame() {}

#endif // Q_OS_WINDOWS
