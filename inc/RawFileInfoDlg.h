#pragma once

#include "ui_RawFileInfoDlg.h"
#include <QDialog>

class RawFileInfoDlg : public QDialog
{
public:
    enum BayerPatternType
    {
        RGGB = 0,
        GRBG = 1,
        GBRG = 2,
        BGGR = 3,
        BAYER_UNKNOW = -1
    };
    enum BayerPixelType
    {
        PIX_R = 0,
        PIX_GR = 1,
        PIX_GB = 2,
        PIX_B = 3,
    };
    enum ByteOrderType
    {
        RAW_LITTLE_ENDIAN = 0,
        RAW_BIG_ENDIAN = 1,
    };

public:
    RawFileInfoDlg() = delete;
    RawFileInfoDlg(QWidget *parent);
    ~RawFileInfoDlg();

public:
    Ui::RawFileInfoDialog ui;
};
