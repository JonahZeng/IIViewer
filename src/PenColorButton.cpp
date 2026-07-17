#include "PenColorButton.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>

namespace
{
constexpr int buttonSize = 28;
constexpr qreal borderMargin = 3.0;
constexpr qreal hoverRingOffset = 2.5;
constexpr qreal hoverRingWidth = 2.0;
constexpr qreal borderWidth = 1.0;
constexpr qreal pressedScale = 0.80;
constexpr int pressDuration = 120;
constexpr int releaseDuration = 220;
constexpr int hoverColorR = 65;
constexpr int hoverColorG = 167;
constexpr int hoverColorB = 228;
}

PenColorButton::PenColorButton(QWidget *parent)
    : QPushButton(parent),
      m_animation(new QPropertyAnimation(this, "pressScale", this))
{
    setFixedSize(buttonSize, buttonSize);
    setCursor(Qt::PointingHandCursor);
}

qreal PenColorButton::pressScale() const
{
    return m_pressScale;
}

void PenColorButton::setPressScale(qreal scale)
{
    m_pressScale = scale;
    update();
}

void PenColorButton::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal side = qMin(width(), height());
    const QPointF center(width() / 2.0, height() / 2.0);
    const qreal baseRadius = side / 2.0 - borderMargin;
    const qreal radius = baseRadius * m_pressScale;

    // Hover highlight ring
    if (underMouse() && isEnabled())
    {
        painter.setPen(QPen(QColor(hoverColorR, hoverColorG, hoverColorB), hoverRingWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(center, radius + hoverRingOffset, radius + hoverRingOffset);
    }

    // Color fill from icon, clipped to circle
    if (!icon().isNull())
    {
        QPainterPath clipPath;
        clipPath.addEllipse(center, radius, radius);
        painter.setClipPath(clipPath);
        const QSize pixSize(static_cast<int>(radius * 2), static_cast<int>(radius * 2));
        const QPixmap pix = icon().pixmap(pixSize);
        const QRectF target(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
        painter.drawPixmap(target, pix, QRectF(0, 0, pix.width(), pix.height()));
        painter.setClipping(false);
    }

    // Border ring
    painter.setPen(QPen(palette().color(QPalette::Mid), borderWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, radius, radius);
}

void PenColorButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_animation->stop();
        m_animation->setDuration(pressDuration);
        m_animation->setStartValue(m_pressScale);
        m_animation->setEndValue(pressedScale);
        m_animation->setEasingCurve(QEasingCurve::InQuad);
        m_animation->start();
    }
    QPushButton::mousePressEvent(event);
}

void PenColorButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_animation->stop();
        m_animation->setDuration(releaseDuration);
        m_animation->setStartValue(m_pressScale);
        m_animation->setEndValue(1.0);
        m_animation->setEasingCurve(QEasingCurve::OutBack);
        m_animation->start();
    }
    QPushButton::mouseReleaseEvent(event);
}
