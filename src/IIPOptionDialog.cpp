#include "IIPOptionDialog.h"
#include <QColorDialog>


IIPOptionDialog::IIPOptionDialog(QWidget *parent): QDialog(parent), ui()
{
    ui.setupUi(this);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::NONE, QIcon()); 
    QPixmap whiteColorIcon(32, 32);
    whiteColorIcon.fill(Qt::white);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::WHITE, QIcon(whiteColorIcon));
    QPixmap grayColorIcon(32, 32);
    grayColorIcon.fill(Qt::gray);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::GRAY, QIcon(grayColorIcon));
    QPixmap blackColorIcon(32, 32);
    blackColorIcon.fill(Qt::black);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::BLACK, QIcon(blackColorIcon));
    QPixmap redColorIcon(32, 32);
    redColorIcon.fill(Qt::red);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::RED, QIcon(redColorIcon));
    QPixmap yellowColorIcon(32, 32);
    yellowColorIcon.fill(Qt::yellow);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::YELLOW, QIcon(yellowColorIcon));
    QPixmap greenColorIcon(32, 32);
    greenColorIcon.fill(Qt::green);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::GREEN, QIcon(greenColorIcon));
    QPixmap cyanColorIcon(32, 32);
    cyanColorIcon.fill(Qt::cyan);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::CYAN, QIcon(cyanColorIcon));
    QPixmap blueColorIcon(32, 32);
    blueColorIcon.fill(Qt::blue);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::BLUE, QIcon(blueColorIcon));
    QPixmap magentaColorIcon(32, 32);
    magentaColorIcon.fill(Qt::magenta);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::MAGENTA, QIcon(magentaColorIcon));
    QPixmap cusColorIcon(32, 32);
    cusColorIcon.fill(pix_val_cus_bg_color);
    ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::CUSTOM, QIcon(cusColorIcon));

    connect(ui.uv_pix_value_mode_comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]()
            { this->uv_disp_mode = ui.uv_pix_value_mode_comboBox->currentIndex();});
    
    connect(ui.paint_pix_val_bg_color_comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &IIPOptionDialog::change_pix_val_bg_color);
}

IIPOptionDialog::~IIPOptionDialog()
{

}

void IIPOptionDialog::change_pix_val_bg_color(int idx)
{
    pix_val_bg_color_index = (PaintPixValBgColor)idx;
    if(pix_val_bg_color_index == PaintPixValBgColor::CUSTOM && this->isVisible())
    {
        QColorDialog dlg(this);
        int resp = dlg.exec();
        if(resp == QColorDialog::Accepted)
        {
            pix_val_cus_bg_color = dlg.selectedColor();
            QPixmap cusColorIcon(32, 32);
            cusColorIcon.fill(pix_val_cus_bg_color);
            ui.paint_pix_val_bg_color_comboBox->setItemIcon(PaintPixValBgColor::CUSTOM, cusColorIcon);
        }
    }
}