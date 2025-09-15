#pragma once

#include "ui_RawFileInfoDlg.h"
#include <QDialog>

class RawFileInfoDlg final : public QDialog
{
public:
    RawFileInfoDlg() = delete;
    explicit RawFileInfoDlg(QWidget *parent);
    ~RawFileInfoDlg();

public:
    Ui::RawFileInfoDialog ui;
};
