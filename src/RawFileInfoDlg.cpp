#include "RawFileInfoDlg.h"
// #include <QValidator>

RawFileInfoDlg::RawFileInfoDlg(QWidget* parent)
    : QDialog(parent)
    , ui()
{
    ui.setupUi(this);
    //    QIntValidator* validator = new QIntValidator(0, 99999, this);
    //    ui.WidthLineEdit->setValidator(validator);
    //    ui.HeightLineEdit->setValidator(validator);
}
