#include "YuvFileInfoDlg.h"

YuvFileInfoDlg::YuvFileInfoDlg(QWidget* parent): QDialog(parent), ui()
{
    ui.setupUi(this);
    QIntValidator* validator = new QIntValidator(0, 99999, this);
    ui.widthLineEdit->setValidator(validator);
    ui.heightLineEdit->setValidator(validator);
}

YuvFileInfoDlg::~YuvFileInfoDlg()
{

}
