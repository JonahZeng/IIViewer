#pragma once
#include <QDialog>
#include <QPushButton>
#include <QLabel>

class AboutDlg final : public QDialog
{
    Q_OBJECT
public:
    AboutDlg() = delete;
    explicit AboutDlg(QWidget *parent);
    ~AboutDlg() = default;
public:
    void onCopyInfo();

private:
    void initUI();
    QPushButton* copy_btn;
    QLabel* label;
};
