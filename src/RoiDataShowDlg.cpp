#include "RoiDataShowDlg.h"
#include <QVBoxLayout>

RoiDataShowDlg::RoiDataShowDlg(QWidget *parent): QDialog(parent), roiInfoEdit(nullptr), closeBtn(nullptr)
{
    initUI();
    connect(closeBtn, &QPushButton::clicked, this, &RoiDataShowDlg::onCloseBtn);
}

void RoiDataShowDlg::initUI()
{
    QVBoxLayout* vlayout = new QVBoxLayout();
    roiInfoEdit = new QTextEdit(this);
    closeBtn = new QPushButton(tr("close"), this);

    vlayout->addWidget(roiInfoEdit, 1);
    vlayout->addWidget(closeBtn, 0);

    setLayout(vlayout);
    resize(640, 480); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    setWindowTitle(tr("Roi data"));
}

void RoiDataShowDlg::onCloseBtn()
{
    close();
}

void RoiDataShowDlg::setRoiExportText(const QString& info)
{
    roiInfoEdit->setText(info);
}
