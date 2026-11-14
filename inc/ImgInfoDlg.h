#ifndef IMG_INFO_DLG_H
#define IMG_INFO_DLG_H
#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include "common_type.h"

class ImgInfoDlg final : public QDialog
{
    Q_OBJECT
public:
    explicit ImgInfoDlg(QWidget *parent);

    void setImgInfo(const QString &filePath, const QSize &imgSize, BayerPatternType bay, YuvType yuv_tp, int bits, bool left);

private:
    struct ImageInfoCard
    {
        QTextEdit *fileName;
        QTextEdit *filePath;
        QLabel *dimensions;
        QLabel *bitDepth;
        QLabel *bayerPattern;
        QLabel *yuvFormat;
    };

    QWidget *createImageCard(const QString &title, ImageInfoCard &card);

    ImageInfoCard leftCard;
    ImageInfoCard rightCard;
};
#endif // IMG_INFO_DLG_H
