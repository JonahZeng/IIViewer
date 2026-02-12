#pragma once
#include "common_type.h"
#include "IIPOptionDialog.h"
#include <QJsonObject>
#include <QString>
#include <QColor>

class AppSettings
{
public:
    AppSettings();
    ~AppSettings() = default;
    QString workPath;
    YuvType yuvType;
    int yuv_bitDepth;
    int yuv_width;
    int yuv_height;
    BayerPatternType rawByType;
    int raw_bitDepth;
    int raw_width;
    int raw_height;
    ByteOrderType rawByteOrder;
    bool raw_compact;
    QString theme;
    int uv_value_disp_mode;
    IIPOptionDialog::PaintPixValBgColor pix_val_bg_index;
    QColor pix_val_cus_bg_color;
    bool workAreaDoubleImgMode;
    QRect windowGeometry;
    QString windowScreenName;
    bool windowMaximized;
    QColor penColor;

    bool loadSettingsFromFile();
    void dumpSettingsToFile();

private:
    void read(const QJsonObject &json);
    void write(QJsonObject &json) const;
};

