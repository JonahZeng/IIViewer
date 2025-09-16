#pragma once
#include <QDialog>
#include <QLabel>
#include "common_type.h"

class ImgInfoDlg final : public QDialog
{
    Q_OBJECT
public:
    explicit ImgInfoDlg(QWidget *parent);
    ~ImgInfoDlg()
    {
    }

    void setImgInfo(QString &filePath, QSize &imgSize, BayerPatternType by, YuvType yt, int bits, bool left);
    
private:
    QLabel *leftLabel;
    QLabel *rightLabel;
};