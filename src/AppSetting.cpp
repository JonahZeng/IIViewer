#include "AppSetting.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStyleFactory>

AppSettings::AppSettings() : yuvType(YuvType::YUV444_INTERLEAVE), 
    yuv_bitDepth(8), // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    yuv_width(0),
    yuv_height(0),
    rawByType(BayerPatternType::RGGB),
    raw_bitDepth(8), // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    raw_width(0),
    raw_height(0),
    rawByteOrder(ByteOrderType::RAW_LITTLE_ENDIAN),
    raw_compact(false),
    uv_value_disp_mode(0), // 0:0=gray 1: half-max=gray 
    pix_val_bg_index(IIPOptionDialog::PaintPixValBgColor::RED),
    pix_val_cus_bg_color(QColor(0, 0, 0)),
    workAreaDoubleImgMode(false),
    windowMaximized(false)
{
    workPath = QDir::homePath();
    theme = QStyleFactory::keys().first();
    windowGeometry = QRect(0, 0, 800, 600);  // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
}

bool AppSettings::loadSettingsFromFile()
{
    const QString appName = QCoreApplication::applicationName();
    const QString appConfigDir = QString(".") + appName;
    const QChar sep = QDir::separator();

    const QDir directory(QDir::homePath());
    if (!directory.exists(appConfigDir))
    {
        directory.mkpath(appConfigDir);
    }
    const QString targetJsonPath = directory.filePath(appConfigDir + sep + appName + ".json");
    const QFileInfo info(targetJsonPath);
    if (info.exists() && info.isFile())
    {
        QFile json_file(targetJsonPath);
        if (json_file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            auto context = json_file.readAll();
            read(QJsonDocument::fromJson(context).object());
            json_file.close();
            return true;
        }
        return false;
    }
    return false;
}

void AppSettings::dumpSettingsToFile()
{
    const QString appName = QCoreApplication::applicationName();
    const QString appConfigDir = QString(".") + appName;
    const QChar sep = QDir::separator();

    const QDir directory(QDir::homePath());
    if (!directory.exists(appConfigDir))
    {
        directory.mkpath(appConfigDir);
    }
    const QString targetJsonPath = directory.filePath(appConfigDir + sep + appName + ".json");

    QFile json_f(targetJsonPath);
    if (json_f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QJsonObject targetObj;
        write(targetObj);
        json_f.write(QJsonDocument(targetObj).toJson());
        json_f.close();
    }
}

void AppSettings::read(const QJsonObject& json) // NOLINT(readability-function-cognitive-complexity)
{
    if (json.contains("workPath"))
    {
        workPath = json["workPath"].toString();
    }
    if (json.contains("yuvType"))
    {
        yuvType = YuvType(json["yuvType"].toInt());
    }
    if (json.contains("yuv_bitDepth"))
    {
        yuv_bitDepth = json["yuv_bitDepth"].toInt();
    }
    if (json.contains("yuv_width"))
    {
        yuv_width = json["yuv_width"].toInt();
    }
    if (json.contains("yuv_height"))
    {
        yuv_height = json["yuv_height"].toInt();
    }
    if (json.contains("rawByType"))
    {
        rawByType = BayerPatternType(json["rawByType"].toInt());
    }
    if (json.contains("raw_bitDepth"))
    {
        raw_bitDepth = json["raw_bitDepth"].toInt();
    }
    if (json.contains("raw_width"))
    {
        raw_width = json["raw_width"].toInt();
    }
    if (json.contains("raw_height"))
    {
        raw_height = json["raw_height"].toInt();
    }
    if (json.contains("raw_byte_order"))
    {
        rawByteOrder = ByteOrderType(json["raw_byte_order"].toInt());
    }
    if (json.contains("raw_compact"))
    {
        raw_compact = json["raw_compact"].toBool();
    }
    if (json.contains("theme"))
    {
        theme = json["theme"].toString();
    }
    if (json.contains("uv_value_disp_mode"))
    {
        uv_value_disp_mode = json["uv_value_disp_mode"].toInt();
    }
    if (json.contains("pix_val_bg_index"))
    {
        pix_val_bg_index = (IIPOptionDialog::PaintPixValBgColor)json["pix_val_bg_index"].toInt();
    }
    if (json.contains("pix_val_cus_bg_color"))
    {
        pix_val_cus_bg_color.setNamedColor(json["pix_val_cus_bg_color"].toString());
    }
    if (json.contains("workAreaDoubleImgMode"))
    {
        workAreaDoubleImgMode = json["workAreaDoubleImgMode"].toBool();
    }
    if (json.contains("windowGeometry"))
    {
        QJsonObject winGeometryObj = json["windowGeometry"].toObject();
        if(winGeometryObj.contains("x") && winGeometryObj.contains("y") && winGeometryObj.contains("width") && winGeometryObj.contains("height"))
        {
            windowGeometry = QRect(winGeometryObj["x"].toInt(), winGeometryObj["y"].toInt(), winGeometryObj["width"].toInt(), winGeometryObj["height"].toInt());
        }
    }
    if (json.contains("windowScreenName"))
    {
        windowScreenName = json["windowScreenName"].toString();
    }
    if (json.contains("windowMaximized"))
    {
        windowMaximized = json["windowMaximized"].toBool();
    }
    if (json.contains("penColor"))
    {
        QJsonObject colorObj = json["penColor"].toObject();
        if (colorObj.contains("r") && colorObj.contains("g") &&  colorObj.contains("b"))
        {
            penColor.setRgb(
                colorObj["r"].toInt(),
                colorObj["g"].toInt(),
                colorObj["b"].toInt()
            );
        }
    }
}

void AppSettings::write(QJsonObject& json) const
{
    json["workPath"] = workPath;

    json["yuvType"] = int(yuvType);

    json["yuv_bitDepth"] = yuv_bitDepth;

    json["yuv_width"] = yuv_width;

    json["yuv_height"] = yuv_height;

    json["rawByType"] = int(rawByType);

    json["raw_bitDepth"] = raw_bitDepth;

    json["raw_width"] = raw_width;

    json["raw_height"] = raw_height;

    json["raw_byte_order"] = int(rawByteOrder);

    json["raw_compact"] = (int)raw_compact;

    json["theme"] = theme;

    json["uv_value_disp_mode"] = uv_value_disp_mode;

    json["pix_val_bg_index"] = (int)pix_val_bg_index;

    json["pix_val_cus_bg_color"] = pix_val_cus_bg_color.name(QColor::HexArgb);

    json["workAreaDoubleImgMode"] = workAreaDoubleImgMode;

    QJsonObject winGeometryObj;
    winGeometryObj["x"] = windowGeometry.left();
    winGeometryObj["y"] = windowGeometry.top();
    winGeometryObj["width"] = windowGeometry.width();
    winGeometryObj["height"] = windowGeometry.height();
    json["windowGeometry"] = winGeometryObj;

    json["windowScreenName"] = windowScreenName;
    json["windowMaximized"] = windowMaximized;
    
    QJsonObject penColorObj;
    penColorObj["r"] = penColor.red();
    penColorObj["g"] = penColor.green();
    penColorObj["b"] = penColor.blue();
    json["penColor"] = penColorObj;
}
