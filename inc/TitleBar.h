#pragma once

#include <QtGlobal>
#include <QWidget>

class QLabel;
class QMenuBar;
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

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void updateButtonIcons(bool maximized);

    QLabel *iconLabel;
    QMenuBar *menuBarWidget;
    QPushButton *minimizeButton;
    QPushButton *maximizeButton;
    QPushButton *closeButton;
    qreal scale;
};
