#pragma once
#include "common_type.h"
#include "IIVOptionDialog.h"
#include <QJsonObject>
#include <QString>
#include <QColor>
#include <QDateTime>
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
    int rgb_bitDepth;
    int rgb_width;
    int rgb_height;
    BayerPatternType rawByType;
    int raw_bitDepth;
    int raw_width;
    int raw_height;
    ByteOrderType rawByteOrder;
    bool raw_compact;
    QString theme;
    IIVOptionDialog::ColorScheme colorScheme;
    int uv_value_disp_mode;
    IIVOptionDialog::PaintPixValBgColor pix_val_bg_index;
    QColor pix_val_cus_bg_color;
    QString uiFontFamily;
    int uiFontPointSize;
    bool workAreaDoubleImgMode;
    QRect windowGeometry;
    QString windowScreenName;
    bool windowMaximized;
    QColor penColor;
    bool dataAnalyseDockVisible;
    bool playListDockVisible;
    QDateTime lastUpdateCheckTime; // 上次成功检查更新的时间，无效表示从未检查过

    bool loadSettingsFromFile();
    void dumpSettingsToFile();

private:
    void read(const QJsonObject &json);
    void write(QJsonObject &json) const;
};

