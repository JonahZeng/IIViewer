#include "AboutDlg.h"
#include "config.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

AboutDlg::AboutDlg(QWidget* parent)
    : QDialog(parent)
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
    log->setMinimumSize(QSize(64, 64));
    QString title = QString(u8"<h1>ISP intermediate photo viewer</h1>");
    QString builder_version = QString(u8"<p>Build by %1, version: %2.%3.%4, branch: %5</p>").arg(GIT_USER).arg(IIViewer_VERSION_MAJOR).arg(IIViewer_VERSION_MINOR).arg(IIViewer_VERSION_PATCH).arg(GIT_BRANCH);
    QString compiler_version = QString(u8"<p>Compiler %1, version: %2, Qt version: %3.%4.%5</p>").arg(CXX_COMPILER_TYPE).arg(CXX_COMPILER_VERSION).arg(QT_VERSION_MAJOR).arg(QT_VERSION_MINOR).arg(QT_VERSION_PATCH);
    QString commith_ash = QString(u8"<p>Commit: %1</p>").arg(GIT_HASH);
    QLabel* label = new QLabel(title + builder_version + compiler_version + commith_ash + u8"<p>Feedback: send e-mail to <a href=\"mailto:zengyangqiao@126.com\">author</a></p>"
                                                                      u8"<p>All right reserved  &#169;2022~2024</p>", this);
    label->setOpenExternalLinks(true);
    QFrame* hline = new QFrame(this);
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);
    vlayout->addWidget(log, 0, Qt::AlignHCenter);
    vlayout->addWidget(label, 0, Qt::AlignHCenter);
    vlayout->addWidget(hline);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addStretch(2);
    QPushButton* ok_btn = new QPushButton("OK", this);
    hlayout->addWidget(ok_btn);

    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    setWindowTitle("About");
    resize(450, 300);

    connect(ok_btn, &QPushButton::clicked, this, &AboutDlg::accept);
}
