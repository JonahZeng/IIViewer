#pragma once
#include <QDialog>

class AboutDlg : public QDialog
{
    Q_OBJECT
public:
    AboutDlg() = delete;
    AboutDlg(QWidget *parent);
    ~AboutDlg();

private:
    void initUI();
};
