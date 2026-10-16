#pragma once
#include "ui_RgbFileInfoDlg.h"
#include <QDialog>

class RgbFileInfoDlg final : public QDialog
{
public:
    RgbFileInfoDlg() = delete;
    explicit RgbFileInfoDlg(QWidget *parent);
    ~RgbFileInfoDlg() = default;

public:
    Ui::rgbInfoDialog ui;
};
