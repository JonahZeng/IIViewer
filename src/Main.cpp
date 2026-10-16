#include "IIViewer.h"
#include <QApplication>
#include <QMessageBox>
// #include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QPalette>
#include <QStyleHints>
#include <QTranslator>
#include <QLibraryInfo>

#ifdef Q_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
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
    // QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#ifdef Q_OS_MACOS
    // Force macOS to show scrollbars always for this app, overriding the
    // system "Show scroll bars" preference. Qt's QMacStyle maps
    // ScrollBarAlwaysOn through NSScroller.preferredScrollerStyle, which
    // respects the system-level setting by default. Setting this
    // app-specific default ensures scrollbars are always visible.
    CFPreferencesSetAppValue(CFSTR("AppleShowScrollBars"), CFSTR("Always"),
                             kCFPreferencesCurrentApplication);
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
#endif
    const QApplication app(argc, argv);
    // Pin the UI to the light color scheme by default so it does not follow
    // the OS light/dark switching (Qt >= 6.5). The scheme saved in the app
    // settings is applied later, when IIViewer loads its settings.
    // Qt < 6.5 has no dark-mode support and is always light.
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Light);
#endif
    // QCoreApplication::setOrganizationName("IIViewer.org");
    // QCoreApplication::setOrganizationDomain("IIViewer.com");
    QCoreApplication::setApplicationName("IIViewer");
// #ifdef Q_OS_WINDOWS
//     QApplication::setFont(QFont("Microsoft YaHei UI", 10)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
// #endif

    QTranslator qt_translator; 
#ifdef Q_OS_MACOS
    QString qt_locale = detectSystemLanguage();
#else
    const QString qt_locale = QLocale::system().name();
#endif
    // qDebug() << "System language: " << qt_locale;
#if QT_VERSION_MAJOR >= 6
    if(qt_translator.load(QString("qt_%1").arg(qt_locale), QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
#else
    if(qt_translator.load(QString("qt_%1").arg(qt_locale), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
#endif
    {
        QApplication::installTranslator(&qt_translator);
    }

    QTranslator translator;
    const QLocale locale = QLocale::system();
    const QString language = QLocale::languageToString(locale.language());
    // QString localeName = locale.name();
    
    // Check the language name and locale prefix for supported translations.
    const bool isChinese = (language == QString("Chinese")) || 
                     qt_locale.startsWith("zh_") || 
                     qt_locale == "zh";
    const bool isKorean = (language == QString("Korean")) ||
                          qt_locale.startsWith("ko_") ||
                          qt_locale.startsWith("ko-") ||
                          qt_locale == "ko";
  
    if(isChinese || isKorean)
    {
        const QDir appDir(QApplication::applicationDirPath());
        // qDebug() << "Application directory: " << appDir.absolutePath();
        const bool isTraditionalChinese = qt_locale.startsWith("zh_TW") ||
                                          qt_locale.startsWith("zh_HK") ||
                                          qt_locale.startsWith("zh_MO") ||
                                          qt_locale == "zh-Hant";
        const QString qmFileName = isKorean
                                       ? QString("translations/IIViewer_ko.qm")
                                       : (isTraditionalChinese
                                              ? QString("translations/IIViewer_zh_TW.qm")
                                              : QString("translations/IIViewer_zh.qm"));
        QString translateFilePath;

#ifdef Q_OS_MACOS
        QDir resourcesDir(appDir);
        if (appDir.dirName() == "MacOS")
        {
            // 如果在.app bundle中，转到Resources目录
            if (resourcesDir.cdUp() && resourcesDir.cd("Resources"))
            {
                translateFilePath = resourcesDir.absoluteFilePath(qmFileName);
            }
            else
            {
                translateFilePath = appDir.absoluteFilePath(QString("../Resources/") + qmFileName);
            }
        } else {
            // 如果不是在.app bundle中，使用相对路径
            translateFilePath = appDir.absoluteFilePath(qmFileName);
        }
#else
        translateFilePath = appDir.absoluteFilePath(qmFileName);
#endif
        if(translator.load(translateFilePath))
        {
            QApplication::installTranslator(&translator);
        }
    }

    QString needOpenFilePath{};
    if(argc > 1)
    {
        const QString path = QString::fromLocal8Bit(argv[1]); 
        const QFileInfo fileInfo(path); // 检查路径是否有效 
        if (fileInfo.exists() && fileInfo.isFile())
        { 
            needOpenFilePath = path;
        }
    }

    IIViewer win{needOpenFilePath};
    win.show();
    return QApplication::exec();
}
