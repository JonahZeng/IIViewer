#include "IIVOptionDialog.h"
#include <QApplication>
#include <QColorDialog>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>

namespace {
constexpr int colorIconSize = 32;
constexpr int minUiFontPointSize = 6;
constexpr int maxUiFontPointSize = 32;
constexpr int defaultUiFontPointSize = 10;
}


IIVOptionDialog::IIVOptionDialog(QWidget *parent): QDialog(parent), pix_val_bg_color_index(PaintPixValBgColor::GRAY), ui()
{
    ui.setupUi(this);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::NONE, QIcon()); 
    QPixmap whiteColorIcon(colorIconSize, colorIconSize);
    whiteColorIcon.fill(Qt::white);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::WHITE, QIcon(whiteColorIcon));
    QPixmap grayColorIcon(colorIconSize, colorIconSize);
    grayColorIcon.fill(Qt::gray);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::GRAY, QIcon(grayColorIcon));
    QPixmap blackColorIcon(colorIconSize, colorIconSize);
    blackColorIcon.fill(Qt::black);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::BLACK, QIcon(blackColorIcon));
    QPixmap redColorIcon(colorIconSize, colorIconSize);
    redColorIcon.fill(Qt::red);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::RED, QIcon(redColorIcon));
    QPixmap yellowColorIcon(colorIconSize, colorIconSize);
    yellowColorIcon.fill(Qt::yellow);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::YELLOW, QIcon(yellowColorIcon));
    QPixmap greenColorIcon(colorIconSize, colorIconSize);
    greenColorIcon.fill(Qt::green);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::GREEN, QIcon(greenColorIcon));
    QPixmap cyanColorIcon(colorIconSize, colorIconSize);
    cyanColorIcon.fill(Qt::cyan);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::CYAN, QIcon(cyanColorIcon));
    QPixmap blueColorIcon(colorIconSize, colorIconSize);
    blueColorIcon.fill(Qt::blue);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::BLUE, QIcon(blueColorIcon));
    QPixmap magentaColorIcon(colorIconSize, colorIconSize);
    magentaColorIcon.fill(Qt::magenta);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::MAGENTA, QIcon(magentaColorIcon));
    QPixmap cusColorIcon(colorIconSize, colorIconSize);
    cusColorIcon.fill(pix_val_cus_bg_color);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::CUSTOM, QIcon(cusColorIcon));

    connect(ui.uv_pix_value_mode_comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]()
            { this->uv_disp_mode = ui.uv_pix_value_mode_comboBox->currentIndex();});
    
    connect(ui.paint_pix_val_bg_color_comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &IIVOptionDialog::change_pix_val_bg_color);

    initFontControls();
    set_ui_display_font(QApplication::font());
}

void IIVOptionDialog::change_pix_val_bg_color(int idx)
{
    pix_val_bg_color_index = static_cast<PaintPixValBgColor>(idx);
    if(pix_val_bg_color_index == PaintPixValBgColor::CUSTOM && this->isVisible())
    {
        QColorDialog dlg(this);
        const int resp = dlg.exec();
        if(resp == QColorDialog::Accepted)
        {
            pix_val_cus_bg_color = dlg.selectedColor();
            QPixmap cusColorIcon(colorIconSize, colorIconSize);
            cusColorIcon.fill(pix_val_cus_bg_color);
            ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::CUSTOM, cusColorIcon);
        }
    }
}

void IIVOptionDialog::set_ui_display_font(const QFont &font)
{
    QFont targetFont = font;
    if (targetFont.family().isEmpty())
    {
        targetFont.setFamily(QApplication::font().family());
    }

    int pointSize = targetFont.pointSize();
    if (pointSize <= 0)
    {
        pointSize = QApplication::font().pointSize();
    }
    if (pointSize <= 0)
    {
        pointSize = defaultUiFontPointSize;
    }

    ui_font_family = targetFont.family();
    ui_font_point_size = pointSize;

    if (uiFontComboBox != nullptr)
    {
        uiFontComboBox->setCurrentFont(targetFont);
    }
    if (uiFontSizeSpinBox != nullptr)
    {
        uiFontSizeSpinBox->setValue(ui_font_point_size);
    }
}

void IIVOptionDialog::initFontControls()
{
    uiFontLabel = new QLabel(tr("UI font:"), this);
    uiFontComboBox = new QFontComboBox(this);
    uiFontSizeSpinBox = new QSpinBox(this);
    uiFontSizeSpinBox->setRange(minUiFontPointSize, maxUiFontPointSize);
    uiFontSizeSpinBox->setSuffix(tr(" pt"));

    uiFontFieldWidget = new QWidget(this);
    auto *fontLayout = new QHBoxLayout(uiFontFieldWidget);
    fontLayout->setContentsMargins(0, 0, 0, 0);
    fontLayout->addWidget(uiFontComboBox);
    fontLayout->addWidget(uiFontSizeSpinBox);

    ui.formLayout_2->addRow(uiFontLabel, uiFontFieldWidget);

    connect(uiFontComboBox, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        ui_font_family = font.family();
    });
    connect(uiFontSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this](int pointSize) {
        ui_font_point_size = pointSize;
    });
}