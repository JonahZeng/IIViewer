#pragma once
#include <QDialog>
#include <QPushButton>
#include <QTextEdit>

class ImgDiffReportDlg final : public QDialog
{
    Q_OBJECT
public:
    static constexpr int GOTO_MAX_DIFF_RESULT = QDialog::Accepted + 1;

    ImgDiffReportDlg() = delete;
    explicit ImgDiffReportDlg(QWidget *parent);
    ~ImgDiffReportDlg()
    {
    }
    void setReportInfo(const QString& info);

private:
    void initUI();
    void onGotoMaxDiffClicked();

    QTextEdit* diffInfoEdit;
    QPushButton* gotoMaxDiffBtn;
};
