#pragma once

class IIViewer;

#include "ImageWidget.h"
#include "PenColorButton.h"
#include "TitleBar.h"
#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QToolBar>
#include <array>

namespace Ui
{
    class IIViewerUi
    {
    public:
        IIViewerUi();
        ~IIViewerUi() = default;
        void setupUi(IIViewer *mainWindow);

    public:
        TitleBar *titleBar;
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
        QAction *onlineHelpAction;
        QAction *useRoiToolAction;
        QAction *useMoveToolAction;
        QAction *sysOptionAction;
        PenColorButton *penColorSetBtn;
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
        QPushButton *imageDiffInfoBtn;
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
        // QPushButton *plot_rgb_contourf_line;
        // QPushButton *plot_rgb_hist;
        // QPushButton *plot_yuv_contourf_line;
        // QPushButton *plot_yuv_hist;
        QTableWidget *fileHistoryTable;

    private:
        void createDockWidget0(IIViewer *mainWindow);
        void createDockWidget1(IIViewer *mainWindow);
    };
}
