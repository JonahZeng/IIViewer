#pragma once
#include <QDialog>
#include <QPushButton>
#include <QTextEdit>

class RoiDataExportDlg : public QDialog
{
    Q_OBJECT
public:
    RoiDataExportDlg() = delete;
    explicit RoiDataExportDlg(QWidget *parent);
    ~RoiDataExportDlg();

public slots:
    void onCloseBtn();

private:
    void initUI();
    QTextEdit* dataDetailEdit;
    QPushButton* closeBtn;
};
