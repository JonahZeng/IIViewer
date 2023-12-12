#include "iipviewer.h"
#include <QApplication>
#include <QMessageBox>
#include <QtNetwork/QHostInfo>
#include <QStyleFactory>
// #include <QDebug>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    // QCoreApplication::setOrganizationName("e-genesys");
    // QCoreApplication::setOrganizationDomain("e-genesys.com");
    // QCoreApplication::setApplicationName("IIP viewer");
    QCoreApplication::setApplicationName("IIPviewer");
    app.setFont(QFont("Microsoft YaHei UI", 10));
    auto themes = QStyleFactory::keys();
    // qDebug() << themes;
    QApplication::setStyle(themes[0]);

    QString localDomainName = QHostInfo::localDomainName();
    if (localDomainName != QString("int.egs.com")) {
        QMessageBox::critical(nullptr, "domain error", "can't run this app on you pc", QMessageBox::StandardButton::Ok);
        return -1;
    }

    IIPviewer w;
    w.show();
    return app.exec();
}
