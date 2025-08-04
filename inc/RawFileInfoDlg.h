#pragma once

#include "ui_RawFileInfoDlg.h"
#include <QDialog>

class RawFileInfoDlg final : public QDialog
{
public:
    enum BayerPatternType
    {
        RGGB = 0,
        GRBG = 1,
        GBRG = 2,
        BGGR = 3,
        RGGIR = 4,
        BGGIR = 5,
        GRIRG = 6,
        GBIRG = 7,
        GIRRG = 8,
        GIRBG = 9,
        IRGGR = 10,
        IRGGB = 11,
        MONO = 12,
        BAYER_UNKNOW = -1
    };
    enum BayerPixelType
    {
        PIX_R = 0,
        PIX_GR = 1,
        PIX_GB = 2,
        PIX_B = 3,
        PIX_IR = 4,
        PIX_Y = 5,
        PIX_UNKNOW = -1
    };
    enum ByteOrderType
    {
        RAW_LITTLE_ENDIAN = 0,
        RAW_BIG_ENDIAN = 1,
    };

public:
    RawFileInfoDlg() = delete;
    explicit RawFileInfoDlg(QWidget *parent);
    ~RawFileInfoDlg();

public:
    Ui::RawFileInfoDialog ui;
};
