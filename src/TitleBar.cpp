#include "TitleBar.h"
#include "IconResource.h"

#include <QGuiApplication>
#include <QIcon>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif

namespace
{
constexpr int baseTitleBarHeight = 32;
constexpr int baseIconSize = 16;
constexpr int baseWindowButtonWidth = 46;
constexpr int baseWindowButtonIconSize = 12;
constexpr int baseLeftMargin = 8;
constexpr int baseLayoutSpacing = 4;
}

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent),
      iconLabel(new QLabel(this)),
      menuBarWidget(nullptr),
      minimizeButton(new QPushButton(this)),
      maximizeButton(new QPushButton(this)),
      closeButton(new QPushButton(this)),
      scale(1.0)
{
    setObjectName(QStringLiteral("titleBar"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    iconLabel->setScaledContents(true);

    for (QPushButton *button : {minimizeButton, maximizeButton, closeButton})
    {
        button->setFlat(true);
        button->setFocusPolicy(Qt::NoFocus);
    }

    minimizeButton->setObjectName(QStringLiteral("titleBarMinimizeButton"));
    maximizeButton->setObjectName(QStringLiteral("titleBarMaximizeButton"));
    closeButton->setObjectName(QStringLiteral("titleBarCloseButton"));
    updateButtonIcons(false);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(baseLeftMargin, 0, 0, 0);
    layout->setSpacing(baseLayoutSpacing);
    layout->addWidget(iconLabel, 0, Qt::AlignVCenter);
    layout->addSpacing(baseLayoutSpacing);
    layout->addStretch(1);
    layout->addWidget(minimizeButton, 0, Qt::AlignRight);
    layout->addWidget(maximizeButton, 0, Qt::AlignRight);
    layout->addWidget(closeButton, 0, Qt::AlignRight);

    refreshWindowButtonStyles();
    refreshMenuBarStyles();

    if (auto *window = qobject_cast<QWidget *>(parent))
    {
        setWindowIconPixmap(window->windowIcon());
        connect(window, &QWidget::windowIconChanged, this, &TitleBar::setWindowIconPixmap);
    }

    updateScale(1.0);

    connect(minimizeButton, &QPushButton::clicked, this, [this]() {
        if (QWidget *topLevelWindow = this->window())
        {
#ifdef Q_OS_WINDOWS
            const HWND hwnd = reinterpret_cast<HWND>(topLevelWindow->winId());
            SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
#else
            topLevelWindow->showMinimized();
#endif
        }
    });
    connect(maximizeButton, &QPushButton::clicked, this, [this]() {
        if (QWidget *topLevelWindow = this->window())
        {
#ifdef Q_OS_WINDOWS
            const HWND hwnd = reinterpret_cast<HWND>(topLevelWindow->winId());
            SendMessage(hwnd, WM_SYSCOMMAND,
                topLevelWindow->isMaximized() ? SC_RESTORE : SC_MAXIMIZE, 0);
#else
            if (topLevelWindow->isMaximized())
            {
                topLevelWindow->showNormal();
            }
            else
            {
                topLevelWindow->showMaximized();
            }
#endif
        }
    });
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        if (auto *window = this->window())
        {
#ifdef Q_OS_WINDOWS
            const HWND hwnd = reinterpret_cast<HWND>(window->winId());
            SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
#else
            window->close();
#endif
        }
    });
}

void TitleBar::setMenuBar(QMenuBar *menuBar)
{
    if (menuBarWidget == menuBar)
    {
        return;
    }

    auto *layout = qobject_cast<QHBoxLayout *>(this->layout());
    if (layout == nullptr)
    {
        return;
    }

    if (menuBarWidget != nullptr)
    {
        layout->removeWidget(menuBarWidget);
        menuBarWidget->setParent(nullptr);
    }

    menuBarWidget = menuBar;
    if (menuBarWidget == nullptr)
    {
        return;
    }

    menuBarWidget->setParent(this);
    menuBarWidget->setNativeMenuBar(false);
    menuBarWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    menuBarWidget->setStyleSheet("QMenuBar { background: transparent; border: none; }"
                                 "QMenuBar::item { padding: 4px 8px; margin: 0; }"
                                 "QMenuBar::item:selected { background: rgba(0, 0, 0, 0.08); }");
    syncMenuBarFont();
    layout->insertWidget(2, menuBarWidget, 0, Qt::AlignmentFlag::AlignVCenter);
}

QMenuBar *TitleBar::menuBar() const
{
    return menuBarWidget;
}

void TitleBar::setWindowIconPixmap(const QIcon &icon)
{
    const int iconSize = qRound(baseIconSize * scale);
    iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
}

void TitleBar::updateMaximizeButton(bool maximized)
{
    updateButtonIcons(maximized);
}

void TitleBar::changeEvent(QEvent *event)
{
    if (event != nullptr)
    {
        switch (event->type())
        {
        case QEvent::FontChange:
        case QEvent::ApplicationFontChange:
        case QEvent::StyleChange:
            syncMenuBarFont();
            break;
        case QEvent::PaletteChange:
        case QEvent::ApplicationPaletteChange:
            // Palette flips with the color scheme; refresh the scheme
            // dependent overlays and themed button icons. Guard against
            // reentry: setStyleSheet() inside the handlers re-sends
            // StyleChange/PaletteChange, which would recurse forever.
            if (!refreshingStyles)
            {
                refreshingStyles = true;
                refreshWindowButtonStyles();
                refreshMenuBarStyles();
                if (auto *mainWindow = qobject_cast<QMainWindow *>(window()))
                {
                    updateButtonIcons(mainWindow->isMaximized());
                }
                refreshingStyles = false;
            }
            break;
        default:
            break;
        }
    }

    QWidget::changeEvent(event);
}

void TitleBar::updateScale(qreal scaleFactor)
{
    scale = qMax(scaleFactor, 1.0);

    const int titleBarHeight = qRound(baseTitleBarHeight * scale);

    setFixedHeight(titleBarHeight);
    iconLabel->setFixedSize(baseIconSize, baseIconSize);

    for (QPushButton *button : {minimizeButton, maximizeButton, closeButton})
    {
        button->setFixedSize(baseWindowButtonWidth, titleBarHeight);
        const int buttonIconSize = qRound(baseWindowButtonIconSize * scale);
        button->setIconSize(QSize(buttonIconSize, buttonIconSize));
    }

    if (auto *window = qobject_cast<QWidget *>(parentWidget()))
    {
        setWindowIconPixmap(window->windowIcon());
    }

    syncMenuBarFont();
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (auto *mainWindow = qobject_cast<QMainWindow *>(window()))
        {
#ifdef Q_OS_WINDOWS
            const HWND hwnd = reinterpret_cast<HWND>(mainWindow->winId());
            SendMessage(hwnd, WM_SYSCOMMAND,
                mainWindow->isMaximized() ? SC_RESTORE : SC_MAXIMIZE, 0);
#else
            if (mainWindow->isMaximized())
            {
                mainWindow->showNormal();
            }
            else
            {
                mainWindow->showMaximized();
            }
#endif
        }
    }

    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::updateButtonIcons(bool maximized)
{
    minimizeButton->setIcon(QIcon(iconPath("titlebar-minimize")));
    maximizeButton->setIcon(QIcon(maximized
        ? iconPath("titlebar-restore")
        : iconPath("titlebar-maximize")));
    closeButton->setIcon(QIcon(iconPath("titlebar-close")));
}

void TitleBar::refreshWindowButtonStyles()
{
    // Hover/pressed highlight overlays must flip with the color scheme:
    // a translucent black overlay is invisible on a dark background, and
    // vice versa. Use styleHints() as the source of truth - the same one
    // iconPath() uses - instead of guessing from the palette, which may
    // not have propagated yet when this runs.
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    constexpr bool dark = false; // Qt < 6.5 has no dark-mode support, always light
#endif
    const char *overlayHover = dark ? "rgba(255, 255, 255, 0.12)" : "rgba(0, 0, 0, 0.08)";
    const char *overlayPressed = dark ? "rgba(255, 255, 255, 0.20)" : "rgba(0, 0, 0, 0.16)";

    setStyleSheet(
        QStringLiteral(
        "QWidget#titleBar { background: palette(window); }"
        "QPushButton#titleBarMinimizeButton, QPushButton#titleBarMaximizeButton, QPushButton#titleBarCloseButton {"
        " border: none; background: transparent; padding: 0; }"
        "QPushButton#titleBarMinimizeButton[ncHover=\"true\"], QPushButton#titleBarMaximizeButton[ncHover=\"true\"] { background: %1; }"
        "QPushButton#titleBarCloseButton[ncHover=\"true\"] { background: rgb(232, 17, 35); }"
        "QPushButton#titleBarMinimizeButton:hover, QPushButton#titleBarMaximizeButton:hover { background: %1; }"
        "QPushButton#titleBarCloseButton:hover { background: rgb(232, 17, 35); }"
        "QPushButton#titleBarMinimizeButton:pressed, QPushButton#titleBarMaximizeButton:pressed { background: %2; }"
        "QPushButton#titleBarCloseButton:pressed { background: rgb(180, 0, 0); }")
            .arg(QLatin1String(overlayHover), QLatin1String(overlayPressed)));
}

void TitleBar::refreshMenuBarStyles()
{
    if (menuBarWidget == nullptr)
    {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    constexpr bool dark = false; // Qt < 6.5 has no dark-mode support, always light
#endif
    menuBarWidget->setStyleSheet(QStringLiteral(
        "QMenuBar { background: transparent; border: none; }"
        "QMenuBar::item { padding: 4px 8px; margin: 0; }"
        "QMenuBar::item:selected { background: %1; }")
            .arg(QLatin1String(dark ? "rgba(255, 255, 255, 0.12)" : "rgba(0, 0, 0, 0.08)")));
}

void TitleBar::refreshThemeStyles()
{
    // Called explicitly after a color scheme change; the reentry guard is
    // inherited from changeEvent handling. Also refreshes themed icons.
    refreshWindowButtonStyles();
    refreshMenuBarStyles();
    if (auto *mainWindow = qobject_cast<QMainWindow *>(window()))
    {
        updateButtonIcons(mainWindow->isMaximized());
    }
}

void TitleBar::syncMenuBarFont()
{
    if (menuBarWidget == nullptr)
    {
        return;
    }

    menuBarWidget->setFont(font());
    if (auto *menuBarStyle = menuBarWidget->style(); menuBarStyle != nullptr)
    {
        menuBarStyle->unpolish(menuBarWidget);
        menuBarStyle->polish(menuBarWidget);
    }
    menuBarWidget->updateGeometry();
    menuBarWidget->adjustSize();
    menuBarWidget->update();
}
