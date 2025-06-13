#pragma once

class IIPviewer;

#include "ImageWidget.h"
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QToolBar>
#include <array>

namespace Ui
{
    class IIPviewerUi
    {
    public:
        IIPviewerUi();
        ~IIPviewerUi();
        void setupUi(IIPviewer *mainWindow);

    public:
        QToolBar *toolBar;
        QAction *openFileLeftAction;
        QAction *openFileRightAction;
        QAction *reloadFileLeftAction;
        QAction *reloadFileRightAction;
        QAction *exitAction;
        QAction *closeLeftAction;
        QAction *closeRightAction;
        QAction *dataAnalyseAction;
        QAction *playListAction;
        QAction *workAreaSingleModeAction;
        QAction *workAreaDoubleModeAction;
        QAction *aboutThisAction;
        QAction *checkUpdateAction;
        QAction *useRoiToolAction;
        QAction *useMoveToolAction;
        QAction *sysOptionAction;
        QPushButton *penColorSetBtn;
        QAction* penColorSetAction;
        QSpinBox *penWidthSbox;
        QAction* penWidthAction;
        QVector<QAction *> themes;
        QFrame *mainWidget;
        std::array<QScrollArea *, 2> scrollArea;
        QFrame *scrollAreaCenterFrame;
        std::array<QWidget *, 2> imageLabelContianer;
        std::array<ImageWidget *, 2> imageLabel;
        QLabel *zoomRatioLabel;
        QPushButton *exchangeAreaPreviewBtn;
        QPushButton *imageInfoBtn;
        QDockWidget *dataAnalyseDockWgt;
        QDockWidget *playListDockWgt;
        QPushButton *paintOk0;
        QPushButton *paintOk1;
        QPushButton *syncRight;
        QPushButton *syncLeft;
        QPushButton *clearPaintBtn;
        QLineEdit *start_x_edit0;
        QLineEdit *start_y_edit0;
        QLineEdit *start_x_edit1;
        QLineEdit *start_y_edit1;
        QLineEdit *end_x_edit0;
        QLineEdit *end_y_edit0;
        QLineEdit *end_x_edit1;
        QLineEdit *end_y_edit1;
        QPushButton *plot_rgb_contourf_line;
        QPushButton *plot_rgb_hist;
        QPushButton *plot_yuv_contourf_line;
        QPushButton *plot_yuv_hist;
        QPushButton *plot_hsv_contourf_line;
        QPushButton *plot_hsv_hist;
        QLineEdit *rgb2yuv_mat_0;
        QLineEdit *rgb2yuv_mat_1;
        QLineEdit *rgb2yuv_mat_2;
        QLineEdit *rgb2yuv_mat_3;
        QLineEdit *rgb2yuv_mat_4;
        QLineEdit *rgb2yuv_mat_5;
        QLineEdit *rgb2yuv_mat_6;
        QLineEdit *rgb2yuv_mat_7;
        QLineEdit *rgb2yuv_mat_8;

    private:
        void createDockWidget0(IIPviewer *mainWindow);
        void createDockWidget1(IIPviewer *mainWindow);
    };
}
