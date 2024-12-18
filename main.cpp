#include "iipviewer.h"
#include <QApplication>
#include <QMessageBox>
// #include <QtNetwork/QHostInfo>
#include <QStyleFactory>
// #include <QDebug>
#include <QFileInfo>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    // QCoreApplication::setOrganizationName("IIPviewer.org");
    // QCoreApplication::setOrganizationDomain("IIPviewer.com");
    QCoreApplication::setApplicationName("IIPviewer");
    app.setFont(QFont("Microsoft YaHei UI", 10));
    auto themes = QStyleFactory::keys();
    // qDebug() << themes;
    QApplication::setStyle(themes[0]);

    //    QString localDomainName = QHostInfo::localDomainName();
    //    if (localDomainName != QString("int.egs.com")) {
    //        QMessageBox::critical(nullptr, "domain error", "can't run this app on you pc", QMessageBox::StandardButton::Ok);
    //        return -1;
    //    }
    QString needOpenFilePath{};
    if(argc > 1) {
        QString path = QString::fromLocal8Bit(argv[1]); 
        QFileInfo fileInfo(path); // 检查路径是否有效 
        if (fileInfo.exists() && fileInfo.isFile()) { 
            needOpenFilePath = path;
        }
    }

    IIPviewer w{needOpenFilePath};
    w.show();
    return app.exec();
}
