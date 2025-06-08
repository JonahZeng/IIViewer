#include "IIPOptionDialog.h"
#include <QColorDialog>


IIPOptionDialog::IIPOptionDialog(QWidget *parent): QDialog(parent), ui()
{
    ui.setupUi(this);

    connect(ui.uv_pix_value_mode_comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]()
            { this->uv_disp_mode = ui.uv_pix_value_mode_comboBox->currentIndex();});
    
    connect(ui.paint_pix_val_bg_color_comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &IIPOptionDialog::change_pix_val_bg_color);
}

IIPOptionDialog::~IIPOptionDialog()
{

}

void IIPOptionDialog::change_pix_val_bg_color(int idx)
{
    pix_val_bg_color_index = (PAINT_PIX_VALUE_BG_COLOR)idx;
    if(pix_val_bg_color_index == PAINT_PIX_VALUE_BG_COLOR::CUSTOM)
    {
        QColorDialog dlg(this);
        int resp = dlg.exec();
        if(resp == QColorDialog::Accepted)
        {
            pix_val_cus_bg_color = dlg.selectedColor();
        }
    }
}