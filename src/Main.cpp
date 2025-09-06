#include "IIPviewer.h"
#include <QApplication>
#include <QMessageBox>
// #include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>

#ifdef Q_OS_MACOS
#include <QProcess>

QString detectSystemLanguage()
{
    // 1. 尝试从 macOS 系统设置读取
    QProcess process;
    process.start("defaults", QStringList() << "read" << "NSGlobalDomain" << "AppleLocale");
    process.waitForFinished();
    QString appleLocale = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    
    if (!appleLocale.isEmpty()) {
        return appleLocale;
    }
    
    // 2. 尝试环境变量
    QString lang = qgetenv("LANG");
    if (!lang.isEmpty()) {
        return lang.split('.').first();
    }
    
    // 3. 使用 QLocale 作为备用
    return QLocale::system().name();
}
#endif

int main(int argc, char* argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    // QCoreApplication::setOrganizationName("IIViewer.org");
    // QCoreApplication::setOrganizationDomain("IIViewer.com");
    QCoreApplication::setApplicationName("IIViewer");
#ifdef Q_OS_WINDOWS
    app.setFont(QFont("Microsoft YaHei UI", 10));
#endif

    QTranslator qt_translator; 
#ifdef Q_OS_MACOS
    QString qt_locale = detectSystemLanguage();
#else
    QString qt_locale = QLocale::system().name();
#endif
    // qDebug() << "System language: " << qt_locale;
    if(qt_translator.load(QString("qt_%1").arg(qt_locale), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qt_translator);

    QTranslator translator;
    QLocale locale = QLocale::system();
    QString language = QLocale::languageToString(locale.language());
    // QString localeName = locale.name();
    
    // 更健壮的语言检测：检查中文语言或中文区域设置
    bool isChinese = (language == QString("Chinese")) || 
                     qt_locale.startsWith("zh_") || 
                     qt_locale == "zh";
  
    if(isChinese)
    {
        QDir appDir(QApplication::applicationDirPath());
        // qDebug() << "Application directory: " << appDir.absolutePath();
        QString zhTranslateFilePath;
        
#ifdef Q_OS_MACOS
        QDir resourcesDir(appDir);
        if (appDir.dirName() == "MacOS") {
            // 如果在.app bundle中，转到Resources目录
            if (resourcesDir.cdUp() && resourcesDir.cd("Resources")) {
                zhTranslateFilePath = resourcesDir.absoluteFilePath("translations/IIViewer_zh.qm");
            } else {
                zhTranslateFilePath = appDir.absoluteFilePath("../Resources/translations/IIViewer_zh.qm");
            }
        } else {
            // 如果不是在.app bundle中，使用相对路径
            zhTranslateFilePath = appDir.absoluteFilePath("translations/IIViewer_zh.qm");
        }
#else
        zhTranslateFilePath = appDir.absoluteFilePath("translations/IIViewer_zh.qm");
#endif
        if(translator.load(zhTranslateFilePath)) {
            app.installTranslator(&translator);
        }
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
