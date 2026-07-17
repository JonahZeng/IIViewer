#pragma once

#include <QPushButton>

class QPropertyAnimation;

class PenColorButton final : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal pressScale READ pressScale WRITE setPressScale)

public:
    explicit PenColorButton(QWidget *parent = nullptr);
    ~PenColorButton() override = default;

    [[nodiscard]] qreal pressScale() const;
    void setPressScale(qreal scale);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    qreal m_pressScale = 1.0;
    QPropertyAnimation *m_animation = nullptr;
};
