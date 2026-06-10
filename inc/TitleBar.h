#pragma once

#include <QtGlobal>
#include <QWidget>

class QLabel;
class QMenuBar;
class QEvent;
class QMouseEvent;
class QPushButton;
class QIcon;

class TitleBar final : public QWidget
{
public:
    explicit TitleBar(QWidget *parent = nullptr);
    ~TitleBar() override = default;

    void setMenuBar(QMenuBar *menuBar);
    QMenuBar *menuBar() const;
    void setWindowIconPixmap(const QIcon &icon);
    void updateMaximizeButton(bool maximized);
    void updateScale(qreal scaleFactor);

    QPushButton *getMinimizeButton() const { return minimizeButton; }
    QPushButton *getMaximizeButton() const { return maximizeButton; }
    QPushButton *getCloseButton() const { return closeButton; }

protected:
    void changeEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void syncMenuBarFont();
    void updateButtonIcons(bool maximized);

    QLabel *iconLabel;
    QMenuBar *menuBarWidget;
    QPushButton *minimizeButton;
    QPushButton *maximizeButton;
    QPushButton *closeButton;
    qreal scale;
};
