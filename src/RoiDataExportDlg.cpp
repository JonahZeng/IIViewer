#include "RoiDataExportDlg.h"
#include <QVBoxLayout>

RoiDataExportDlg::RoiDataExportDlg(QWidget *parent): QDialog(parent), dataDetailEdit(nullptr), closeBtn(nullptr)
{
    initUI();
    connect(closeBtn, &QPushButton::clicked, this, &RoiDataExportDlg::onCloseBtn);
}

RoiDataExportDlg::~RoiDataExportDlg()
{

}

void RoiDataExportDlg::initUI()
{
    QVBoxLayout* vlayout = new QVBoxLayout();
    dataDetailEdit = new QTextEdit(this);
    closeBtn = new QPushButton(tr("close"), this);

    vlayout->addWidget(dataDetailEdit, 1);
    vlayout->addWidget(closeBtn, 0);

    setLayout(vlayout);
    setWindowTitle(tr("Roi data"));
}

void RoiDataExportDlg::onCloseBtn()
{
    close();
}
