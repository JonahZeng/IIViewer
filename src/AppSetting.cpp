#include "AppSetting.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStyleFactory>

AppSettings::AppSettings()
{
    workPath = QDir::homePath();
    yuvType = YuvFileInfoDlg::YUV444_INTERLEAVE;
    yuv_bitDepth = 8;
    yuv_width = 0;
    yuv_height = 0;
    rawByType = RawFileInfoDlg::RGGB;
    raw_bitDepth = 8;
    raw_width = 0;
    raw_height = 0;
    rawByteOrder = RawFileInfoDlg::ByteOrderType::RAW_LITTLE_ENDIAN;
    uv_value_disp_mode = 0; // 0:0=gray 1: half-max=gray 
    theme = QStyleFactory::keys().first();
}

AppSettings::~AppSettings()
{
}

bool AppSettings::loadSettingsFromFile()
{
    QString appName = QCoreApplication::applicationName();
    QString appConfigDir = QString(".") + appName;
    const QChar sep = QDir::separator();

    QDir directory(QDir::homePath());
    if (!directory.exists(appConfigDir))
    {
        directory.mkpath(appConfigDir);
    }
    QString targetJsonPath = directory.filePath(appConfigDir + sep + appName + ".json");
    QFileInfo info(targetJsonPath);
    if (info.exists() && info.isFile())
    {
        QFile jf(targetJsonPath);
        if (jf.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            auto context = jf.readAll();
            read(QJsonDocument::fromJson(context).object());
            jf.close();
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void AppSettings::dumpSettingsToFile()
{
    QString appName = QCoreApplication::applicationName();
    QString appConfigDir = QString(".") + appName;
    const QChar sep = QDir::separator();

    QDir directory(QDir::homePath());
    if (!directory.exists(appConfigDir))
    {
        directory.mkpath(appConfigDir);
    }
    QString targetJsonPath = directory.filePath(appConfigDir + sep + appName + ".json");

    QFile jf(targetJsonPath);
    if (jf.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QJsonObject targetObj;
        write(targetObj);
        jf.write(QJsonDocument(targetObj).toJson());
        jf.close();
    }
}

void AppSettings::read(const QJsonObject& json)
{
    if (json.contains("workPath"))
    {
        workPath = json["workPath"].toString();
    }
    if (json.contains("yuvType"))
    {
        yuvType = YuvFileInfoDlg::YuvType(json["yuvType"].toInt());
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
        rawByType = RawFileInfoDlg::BayerPatternType(json["rawByType"].toInt());
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
        rawByteOrder = RawFileInfoDlg::ByteOrderType(json["raw_byte_order"].toInt());
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
        windowGeometry = QRect(
            json["windowGeometry_x"].toInt(),
            json["windowGeometry_y"].toInt(),
            json["windowGeometry_width"].toInt(),
            json["windowGeometry_height"].toInt()
        );
    }
    if (json.contains("windowScreenName"))
    {
        windowScreenName = json["windowScreenName"].toString();
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
    json["windowGeometry"] = !windowGeometry.isNull();
    if (!windowGeometry.isNull())
    {
        json["windowGeometry_x"] = windowGeometry.x();
        json["windowGeometry_y"] = windowGeometry.y();
        json["windowGeometry_width"] = windowGeometry.width();
        json["windowGeometry_height"] = windowGeometry.height();
    }
    json["windowScreenName"] = windowScreenName;
}
