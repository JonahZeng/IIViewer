#pragma once

#include "ui_IIPOptionDialog.h"
#include <QDialog>

class IIPOptionDialog : public QDialog
{
public:
    IIPOptionDialog() = delete;
    IIPOptionDialog(QWidget *parent);
    ~IIPOptionDialog();

public:
    Ui::IIPOptionDialog ui;
};
