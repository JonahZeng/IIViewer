#include "ImgDiffReportDlg.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

ImgDiffReportDlg::ImgDiffReportDlg(QWidget *parent): QDialog(parent), diffInfoEdit(nullptr), gotoMaxDiffBtn(nullptr)
{
    initUI();
}

void ImgDiffReportDlg::initUI()
{
    QVBoxLayout* vlayout = new QVBoxLayout();
    diffInfoEdit = new QTextEdit(this);
    diffInfoEdit->setReadOnly(true);
    gotoMaxDiffBtn = new QPushButton(tr("jump to max diff"), this);
    connect(gotoMaxDiffBtn, &QPushButton::clicked, this, &ImgDiffReportDlg::onGotoMaxDiffClicked);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    btnLayout->addWidget(gotoMaxDiffBtn);

    vlayout->addWidget(diffInfoEdit, 1);
    vlayout->addLayout(btnLayout);

    setLayout(vlayout);
    resize(768, 320); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    setWindowTitle(tr("img difference report"));
}

void ImgDiffReportDlg::setReportInfo(const QString& info)
{
    diffInfoEdit->clear();
    diffInfoEdit->setText(info);
}

void ImgDiffReportDlg::onGotoMaxDiffClicked()
{
    done(GOTO_MAX_DIFF_RESULT);
}
