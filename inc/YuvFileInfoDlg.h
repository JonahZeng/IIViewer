#pragma once
#include "ui_YuvFileInfoDlg.h"
#include <QDialog>

class YuvFileInfoDlg final : public QDialog
{
public:
    YuvFileInfoDlg() = delete;
    explicit YuvFileInfoDlg(QWidget *parent);
    ~YuvFileInfoDlg();

public:
    Ui::yuvInfoDialog ui;
};
