#pragma once
#include "RawFileInfoDlg.h"
#include "YuvFileInfoDlg.h"
#include <QJsonObject>
#include <QString>

class AppSettings {
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

    bool loadSettingsFromFile();
    void dumpSettingsToFile();

private:
    void read(const QJsonObject& json);
    void write(QJsonObject& json) const;
};
