#include "AboutDlg.h"
#include "config.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QClipboard>
#include <QMessageBox>

AboutDlg::AboutDlg(QWidget* parent)
    : QDialog(parent), copy_btn(nullptr), label(nullptr)
{
    initUI();
}

AboutDlg::~AboutDlg()
{
}

void AboutDlg::initUI()
{
    QVBoxLayout* vlayout = new QVBoxLayout();
    QLabel* log = new QLabel(this);
    log->setStyleSheet("QLabel{image: url(:/image/resource/aboutlog.png)}");
    log->setMinimumSize(QSize(96, 96));
    QString title = QString(u8"<h1>ISP intermediate photo viewer</h1>\n");
    QString builder_version = QString(u8"<p>Build by %1, version: %2.%3.%4, branch: %5</p>\n").arg(GIT_USER).arg(IIViewer_VERSION_MAJOR).arg(IIViewer_VERSION_MINOR).arg(IIViewer_VERSION_PATCH).arg(GIT_BRANCH);
    QString compiler_version = QString(u8"<p>Compiler %1, version: %2, Qt version: %3.%4.%5</p>\n").arg(CXX_COMPILER_TYPE).arg(CXX_COMPILER_VERSION).arg(QT_VERSION_MAJOR).arg(QT_VERSION_MINOR).arg(QT_VERSION_PATCH);
    QString commith_ash = QString(u8"<p>Commit: %1</p>\n").arg(GIT_HASH);
    label = new QLabel(title + builder_version + compiler_version + commith_ash + u8"<p>Feedback: send e-mail to <a href=\"mailto:zengyangqiao@126.com\">author</a></p>\n"
                                                                      u8"<p>All right reserved  &#169;2022~2025</p>", this);
    label->setOpenExternalLinks(true);
    QFrame* hline = new QFrame(this);
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);
    vlayout->addWidget(log, 0, Qt::AlignmentFlag::AlignHCenter);
    vlayout->addWidget(label, 0, Qt::AlignmentFlag::AlignHCenter);
    vlayout->addWidget(hline);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addStretch(2);
    copy_btn = new QPushButton(tr("Copy Info"), this);
    QPushButton* ok_btn = new QPushButton(tr("OK"), this);
    hlayout->addWidget(copy_btn);
    hlayout->addWidget(ok_btn);
    ok_btn->setFocus();

    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    setWindowTitle(tr("About"));
    resize(450, 300);

    connect(ok_btn, &QPushButton::clicked, this, &AboutDlg::accept);
    connect(copy_btn, &QPushButton::clicked, this, &AboutDlg::onCopyInfo);
}

void AboutDlg::onCopyInfo()
{
    QString info = label->text();
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(info);
    QMessageBox::information(this, tr("copy version info"), tr("copy info to clipboard successed!"), QMessageBox::StandardButton::Ok);
}
