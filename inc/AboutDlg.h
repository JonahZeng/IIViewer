#pragma once
#include <QDialog>

class AboutDlg : public QDialog
{
public:
    AboutDlg() = delete;
    AboutDlg(QWidget *parent);
    ~AboutDlg();

private:
    void initUI();
};
