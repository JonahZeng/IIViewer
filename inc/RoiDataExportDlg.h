#pragma once
#include <QDialog>
#include <QPushButton>
#include <QTextEdit>

class RoiDataExportDlg final : public QDialog
{
    Q_OBJECT
public:
    RoiDataExportDlg() = delete;
    explicit RoiDataExportDlg(QWidget *parent);
    ~RoiDataExportDlg();
    void setRoiExportText(const QString&);

public slots:
    void onCloseBtn();

private:
    void initUI();
    QTextEdit* roiInfoEdit;
    QPushButton* closeBtn;
};
