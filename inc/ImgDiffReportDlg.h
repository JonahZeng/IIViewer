#pragma once
#include <QDialog>
#include <QTextEdit>

class ImgDiffReportDlg final : public QDialog
{
    Q_OBJECT
public:
    ImgDiffReportDlg() = delete;
    explicit ImgDiffReportDlg(QWidget *parent);
    ~ImgDiffReportDlg()
    {
    }
    void setReportInfo(QString& info);

private:
    void initUI();
    QTextEdit* diffInfoEdit;
};