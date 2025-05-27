#pragma once
#include <QDialog>
#include <QPushButton>
#include <QLabel>

class AboutDlg : public QDialog
{
    Q_OBJECT
public:
    AboutDlg() = delete;
    AboutDlg(QWidget *parent);
    ~AboutDlg();
public:
    void onCopyInfo();

private:
    void initUI();
    QPushButton* copy_btn;
    QLabel* label;
};
