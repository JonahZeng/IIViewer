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
    QLabel* log = new QLabel();
    log->setStyleSheet("QLabel{image: url(:/image/resource/aboutlog.png)}");
    log->setMinimumSize(QSize(64, 64));
    QString title = QString(u8"<h1>ISP intermediate photo viewer</h1>");
    QString builder_version = QString(u8"<p>Build by %1, version: %2.%3.%4</p>").arg(GIT_USER).arg(IIPviewer_VERSION_MAJOR).arg(IIPviewer_VERSION_MINOR).arg(IIPviewer_VERSION_PATCH);
    QString commithash = QString(u8"<p>Commit: %1</p>").arg(GIT_HASH);
    QLabel* label = new QLabel(title + builder_version + commithash + u8"<p>Feedback: sent e-mail to the author by click <a href=\"mailto:json.zeng@e-genesys.com\">json.zeng</a></p>"
                                                                      u8"<p>Geneturino All right reserved  &#169;2022~2023</p>");
    label->setOpenExternalLinks(true);
    QFrame* hline = new QFrame(this);
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);
    vlayout->addWidget(log, 0, Qt::AlignHCenter);
    vlayout->addWidget(label, 0, Qt::AlignHCenter);
    vlayout->addWidget(hline);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addStretch(2);
    QPushButton* ok_btn = new QPushButton("OK");
    hlayout->addWidget(ok_btn);

    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    setWindowTitle("About");
    resize(450, 300);

    connect(ok_btn, &QPushButton::clicked, this, &AboutDlg::accept);
}
