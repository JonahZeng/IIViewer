#ifndef ICONRESOURCE_H
#define ICONRESOURCE_H

#include <QGuiApplication>
#include <QString>
#include <QStyleHints>
#include <QtGlobal>

// Return the resource path of the SVG icon matching the current color
// scheme, e.g. iconPath("file-x") -> ":/image/src/resource/file-x-light.svg".
// Each name must have a name-light.svg and a name-dark.svg in src/resource/.
inline QString iconPath(const QString &name)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    constexpr bool dark = false; // Qt < 6.5 has no dark-mode support, always light
#endif
    return QStringLiteral(":/image/src/resource/%1-%2.svg")
        .arg(name, dark ? QStringLiteral("dark") : QStringLiteral("light"));
}
#endif
