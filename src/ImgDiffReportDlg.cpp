#include "ImgDiffReportDlg.h"
#include <QVBoxLayout>

ImgDiffReportDlg::ImgDiffReportDlg(QWidget *parent): QDialog(parent), diffInfoEdit(nullptr)
{
    initUI();
}

void ImgDiffReportDlg::initUI()
{
    QVBoxLayout* vlayout = new QVBoxLayout();
    diffInfoEdit = new QTextEdit(this);

    vlayout->addWidget(diffInfoEdit, 1);

    setLayout(vlayout);
    resize(768, 320);
    setWindowTitle(tr("img difference report"));
}

void ImgDiffReportDlg::setReportInfo(QString& info)
{
    diffInfoEdit->clear();
    diffInfoEdit->setText(info);
}