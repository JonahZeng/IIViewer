#include "iipviewer.h"
#include <QApplication>
#include <QMessageBox>
// #include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    // QCoreApplication::setOrganizationName("IIViewer.org");
    // QCoreApplication::setOrganizationDomain("IIViewer.com");
    QCoreApplication::setApplicationName("IIViewer");
    app.setFont(QFont("Microsoft YaHei UI", 10));
    // QString country = QLocale::countryToString(locale.country());
    // qDebug() << "Language:" << language;
    // qDebug() << "Country:" << country;
    QTranslator qt_translator; 
    QString qt_locale = QLocale::system().name();
    qt_translator.load(QString("qt_%1").arg(qt_locale), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qt_translator);

    QTranslator translator;
    QLocale locale; 
    QString language = QLocale::languageToString(locale.language());
    if(language == QString("Chinese"))
    {
        QDir appDir(QApplication::applicationDirPath());
        QString zhTranslateFilePath = appDir.filePath("translations/IIViewer_zh.qm");
        
        if(translator.load(zhTranslateFilePath))
            app.installTranslator(&translator);
    }

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
