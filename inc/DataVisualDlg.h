#pragma once
#include "ImageWidget.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QtDataVisualization>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using QtDataVisualization::Q3DSurface;
using QtDataVisualization::QSurface3DSeries;
#endif

class DataVisualDialog : public QDialog
{
    Q_OBJECT
public:
    DataVisualDialog() = delete;
    DataVisualDialog(QWidget *parent, bool prepared, ImageWidget *wg, QPoint st0, QPoint st1);
    ~DataVisualDialog();

private:
    QCheckBox *dispR;
    QCheckBox *dispG;
    QCheckBox *dispB;

    QRadioButton *selectNoItem;
    QRadioButton *selectItem;
    QRadioButton *selectItemAndRow;
    QRadioButton *selectItemAndCol;

    QPushButton *okBtn;
    QPushButton *cancelBtn;
    QSurface3DSeries *series0;
    QSurface3DSeries *series1;
    QSurface3DSeries *series2;

    QButtonGroup selectionGroup;

    Q3DSurface *graph;
};
