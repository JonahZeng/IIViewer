#pragma once
#include <QDialog>
#include <QPushButton>
#include <QTextEdit>

class RoiDataShowDlg final : public QDialog
{
    Q_OBJECT
public:
    RoiDataShowDlg() = delete;
    explicit RoiDataShowDlg(QWidget *parent);
    ~RoiDataShowDlg();
    void setRoiExportText(const QString&);

public slots:
    void onCloseBtn();

private:
    void initUI();
    QTextEdit* roiInfoEdit;
    QPushButton* closeBtn;
};

