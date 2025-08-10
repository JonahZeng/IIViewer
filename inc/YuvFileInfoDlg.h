#pragma once
#include "ui_YuvFileInfoDlg.h"
#include <QDialog>

class YuvFileInfoDlg final : public QDialog
{
public:
    enum YuvType
    {
        YUV444_INTERLEAVE = 0,
        YUV444_PLANAR = 1,
        YUV422_UYVY = 2,
        YUV422_YUYV = 3,
        YUV420_NV12 = 4,
        YUV420_NV21 = 5,
        YUV420P_YU12 = 6,
        YUV420P_YV12 = 7,
        YUV400 = 8,
        YUV_UNKNOW = -1
    };

public:
    YuvFileInfoDlg() = delete;
    explicit YuvFileInfoDlg(QWidget *parent);
    ~YuvFileInfoDlg();

public:
    Ui::yuvInfoDialog ui;
};
