#pragma once
#include <QDialog>
#include "RawFileInfoDlg.h"
#include "YuvFileInfoDlg.h"

class ImgInfoDlg final : public QDialog
{
    Q_OBJECT
public:
    explicit ImgInfoDlg(QWidget *parent);
    ~ImgInfoDlg()
    {
    }

    void setImgInfo(QString &filePath, QSize &imgSize, RawFileInfoDlg::BayerPatternType by, YuvFileInfoDlg::YuvType yt, int bits, bool left);
    
private:
    QLabel *leftLabel;
    QLabel *rightLabel;
};