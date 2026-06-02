#pragma once

#include "ui_IIVOptionDialog.h"
#include <QDialog>
#include <QColor>
#include <QFont>

class QLabel;
class QFontComboBox;
class QSpinBox;
class QWidget;

class IIVOptionDialog final : public QDialog
{
    Q_OBJECT

public:
    enum PaintPixValBgColor {
        NONE = 0,
        WHITE = 1,
        GRAY = 2,
        BLACK = 3,
        RED = 4,
        YELLOW = 5,
        GREEN = 6,
        CYAN = 7,
        BLUE = 8,
        MAGENTA = 9,
        CUSTOM = 10
    };
public:
    IIVOptionDialog() = delete;
    explicit IIVOptionDialog(QWidget *parent);
    ~IIVOptionDialog() = default;

    void set_uv_disp_mode(int mode)
    {
        uv_disp_mode = mode;
        ui.uv_pix_value_mode_comboBox->setCurrentIndex(uv_disp_mode);
    }

    void set_pix_val_bg_color_index(PaintPixValBgColor i)
    {
        pix_val_bg_color_index = i;
        ui.paint_pix_val_bg_color_comboBox->setCurrentIndex(pix_val_bg_color_index);
    }

    void set_pix_val_custom_bg_color(QColor bg)
    {
        pix_val_cus_bg_color = bg;
        QPixmap cusColorIcon(32, 32);
        cusColorIcon.fill(pix_val_cus_bg_color);
        ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::CUSTOM, QIcon(cusColorIcon));
    }

    void set_ui_display_font(const QFont &font);

    PaintPixValBgColor pix_val_bg_color_index;
    QColor pix_val_cus_bg_color;
    int uv_disp_mode;

    QString ui_font_family;
    int ui_font_point_size = -1;

public slots:
    void change_pix_val_bg_color(int idx);

public:
    Ui::IIVOptionDialog ui;

private:
    void initFontControls();

    QLabel *uiFontLabel = nullptr;
    QFontComboBox *uiFontComboBox = nullptr;
    QSpinBox *uiFontSizeSpinBox = nullptr;
    QWidget *uiFontFieldWidget = nullptr;
};
