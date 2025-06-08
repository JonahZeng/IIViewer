#pragma once

#include "ui_IIPOptionDialog.h"
#include <QDialog>
#include <QColor>

class IIPOptionDialog : public QDialog
{
public:
    enum PAINT_PIX_VALUE_BG_COLOR {
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
    IIPOptionDialog() = delete;
    explicit IIPOptionDialog(QWidget *parent);
    ~IIPOptionDialog();

    void set_uv_disp_mode(int mode)
    {
        uv_disp_mode = mode;
    }

    void set_pix_val_bg_color_index(int i)
    {
        pix_val_bg_color_index = (PAINT_PIX_VALUE_BG_COLOR)i;
    }

    void set_pix_val_custom_bg_color(QColor bg)
    {
        pix_val_cus_bg_color = bg;
    }

    PAINT_PIX_VALUE_BG_COLOR pix_val_bg_color_index;
    QColor pix_val_cus_bg_color;
    int uv_disp_mode;

public slots:
    void change_pix_val_bg_color(int idx);

public:
    Ui::IIPOptionDialog ui;
};
