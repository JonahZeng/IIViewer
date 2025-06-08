#pragma once
#include "RawFileInfoDlg.h"
#include "YuvFileInfoDlg.h"
#include <QJsonObject>
#include <QString>
#include <QColor>

class AppSettings
{
public:
    explicit AppSettings();
    ~AppSettings();
    QString workPath;
    YuvFileInfoDlg::YuvType yuvType;
    int yuv_bitDepth;
    int yuv_width;
    int yuv_height;
    RawFileInfoDlg::BayerPatternType rawByType;
    int raw_bitDepth;
    int raw_width;
    int raw_height;
    RawFileInfoDlg::ByteOrderType rawByteOrder;
    bool raw_compact;
    QString theme;
    int uv_value_disp_mode;
    int pix_val_bg_index;
    QColor pix_val_cus_bg_color;

    bool loadSettingsFromFile();
    void dumpSettingsToFile();

private:
    void read(const QJsonObject &json);
    void write(QJsonObject &json) const;
};
