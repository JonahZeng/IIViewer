#include "IIPviewer_ui.h"

#include <QBoxLayout>
#include <QButtonGroup>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QValidator>
#include <QStyleFactory>

#include "iipviewer.h"

Ui::IIPviewerUi::IIPviewerUi() : openFileLeftAction(nullptr), openFileRightAction(nullptr), exitAction(nullptr), 
    closeLeftAction(nullptr), closeRightAction(nullptr), dataAnalyseAction(nullptr), playListAction(nullptr), aboutQtAction(nullptr), 
    aboutThisAction(nullptr), mainWidget(nullptr), scrollArea{nullptr, nullptr}, imageLabel{nullptr, nullptr}, 
    dataAnalyseDockWgt(nullptr), playListDockWgt(nullptr), paintOk0(nullptr), paintOk1(nullptr), 
    syncRight(nullptr), syncLeft(nullptr), clearPaintBtn(nullptr), start_x_edit0(nullptr), start_y_edit0(nullptr), 
    start_x_edit1(nullptr), start_y_edit1(nullptr), end_x_edit0(nullptr), end_y_edit0(nullptr), end_x_edit1(nullptr), 
    end_y_edit1(nullptr), plot_rgb_contourf_line(nullptr), plot_rgb_hist(nullptr), plot_yuv_contourf_line(nullptr), 
    plot_yuv_hist(nullptr), plot_hsv_contourf_line(nullptr), plot_hsv_hist(nullptr), rgb2yuv_mat_0(nullptr), rgb2yuv_mat_1(nullptr), 
    rgb2yuv_mat_2(nullptr), rgb2yuv_mat_3(nullptr), rgb2yuv_mat_4(nullptr), rgb2yuv_mat_5(nullptr), rgb2yuv_mat_6(nullptr), rgb2yuv_mat_7(nullptr), rgb2yuv_mat_8(nullptr)
{
}

Ui::IIPviewerUi::~IIPviewerUi() {}

void Ui::IIPviewerUi::setupUi(IIPviewer *mainWindow)
{
    QMenuBar *mbar = mainWindow->menuBar();
    QMenu *fileMenu = mbar->addMenu("&File");
    toolBar = new QToolBar(mainWindow);
    toolBar->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);
    mainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar);
    // mbar->setStyleSheet("QMenuBar{background:rgb(128,128,128)}
    // QMenuBar::item:selected{background:rgb(128,128,128)}
    // QMenu{background:rgb(128,128,128)}
    // QMenu::item:selected{background-color:#654321;}");
    openFileLeftAction = new QAction("Open files left", mainWindow);
    openFileLeftAction->setIcon(QIcon(":/image/resource/file-earmark-left.png"));
    openFileLeftAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    openFileRightAction = new QAction("Open files right", mainWindow);
    openFileRightAction->setIcon(QIcon(":/image/resource/file-earmark-right.png"));
    openFileRightAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    reloadFileLeftAction = new QAction("Reload left image", mainWindow);
    reloadFileLeftAction->setIcon(QIcon(":/image/resource/reload-left.png"));
    reloadFileRightAction = new QAction("Reload right image", mainWindow);
    reloadFileRightAction->setIcon(QIcon(":/image/resource/reload-right.png"));
    exitAction = new QAction("Exit", mainWindow);
    exitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    closeLeftAction = new QAction("Close left image", mainWindow);
    closeLeftAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));
    closeLeftAction->setIcon(QIcon(":/image/resource/file-x.svg"));
    closeRightAction = new QAction("Close right image", mainWindow);
    closeRightAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    closeRightAction->setIcon(QIcon(":/image/resource/file-x.svg"));
    fileMenu->addAction(openFileLeftAction);
    fileMenu->addAction(openFileRightAction);
    fileMenu->addAction(reloadFileLeftAction);
    fileMenu->addAction(reloadFileRightAction);
    fileMenu->addAction(closeLeftAction);
    fileMenu->addAction(closeRightAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    QMenu *viewMenu = mbar->addMenu("&View");
    QMenu *dockWidgetMenu = new QMenu("panal", mainWindow);
    // dockWidgetMenu->setStyleSheet("QMenu{background:rgb(128,128,128)}QMenu::item:selected{background-color:#654321;}");
    viewMenu->addMenu(dockWidgetMenu);
    dataAnalyseAction = new QAction("data analyse", mainWindow);
    playListAction = new QAction("play list", mainWindow);
    dockWidgetMenu->addAction(dataAnalyseAction);
    dockWidgetMenu->addAction(playListAction);
    dataAnalyseAction->setCheckable(true);
    dataAnalyseAction->setChecked(true);
    playListAction->setCheckable(true);
    playListAction->setChecked(true);

    QMenu *themeMenu = new QMenu("theme", mainWindow);
    auto themeStrings = QStyleFactory::keys();
    for (auto &theme : themeStrings)
    {
        auto themePtr = new QAction(theme, mainWindow);
        themePtr->setCheckable(true);
        if (theme == themeStrings.first())
        {
            themePtr->setChecked(true);
        }
        else
        {
            themePtr->setChecked(false);
        }

        themes.push_back(themePtr);
        themeMenu->addAction(themePtr);
    }

    QMenu *settingMenu = mbar->addMenu("&Setting");
    QMenu *toolBoxMenu = new QMenu("toolBox", mainWindow);
    useRoiToolAction = new QAction("pen", mainWindow);
    useRoiToolAction->setCheckable(true);
    useRoiToolAction->setChecked(false);
    useRoiToolAction->setIcon(QIcon(":/image/resource/bounding-box-circles.svg"));
    useMoveToolAction = new QAction("move", mainWindow);
    useMoveToolAction->setCheckable(true);
    useMoveToolAction->setChecked(false);
    useMoveToolAction->setIcon(QIcon(":image/resource/arrows-move.svg"));
    toolBoxMenu->addAction(useMoveToolAction);
    toolBoxMenu->addAction(useRoiToolAction);
    settingMenu->addMenu(toolBoxMenu);
    settingMenu->addMenu(themeMenu);

    QMenu *aboutMenu = mbar->addMenu("&About");
    aboutQtAction = new QAction("About Qt", mainWindow);
    aboutThisAction = new QAction("About | Feedback", mainWindow);
    aboutThisAction->setIcon(QIcon(":/image/resource/info-circle.svg"));
    aboutMenu->addAction(aboutQtAction);
    aboutMenu->addAction(aboutThisAction);

    toolBar->addAction(openFileLeftAction);
    toolBar->addAction(openFileRightAction);
    toolBar->addAction(reloadFileLeftAction);
    toolBar->addAction(reloadFileRightAction);
    toolBar->addAction(closeLeftAction);
    toolBar->addAction(closeRightAction);
    toolBar->addSeparator();
    toolBar->addAction(aboutThisAction);
    toolBar->addSeparator();
    toolBar->addAction(useMoveToolAction);
    toolBar->addAction(useRoiToolAction);
    toolBar->addSeparator();
    penColorSetBtn = new QPushButton(toolBar);
    QPixmap penColorIcon(32, 32);
    penColorIcon.fill(QColor(0, 0, 0));
    penColorSetBtn->setIcon(penColorIcon);
    penColorSetBtn->setStyleSheet("QPushButton {border: none;}"
                                  "QPushButton:hover{background-color:#41a7e0} "
                                  "QPushButton:pressed{background-color:#a7a7a7}");
    penColorSetBtn->setMinimumWidth(64);
    penColorSetBtn->setVisible(false);
    penColorSetBtn->setEnabled(true);
    penColorSetAction = toolBar->addWidget(penColorSetBtn);
    
    penWidthSbox = new QSpinBox(toolBar);
    penWidthSbox->setRange(1, 32);
    penWidthSbox->setSingleStep(1);
    penWidthSbox->setStepType(QSpinBox::StepType::DefaultStepType);
    penWidthSbox->setSuffix("px");
    penWidthSbox->setEnabled(true);
    penWidthSbox->setValue(2);
    penWidthSbox->setVisible(false);
    penWidthAction = toolBar->addWidget(penWidthSbox);
    

    mainWidget = new QFrame();
    //    mainWidget->setFrameStyle(QFrame::Box | QFrame::Sunken);
    mainWidget->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    QHBoxLayout *scrollAreaLayout = new QHBoxLayout();
    scrollAreaLayout->setContentsMargins(0, 0, 0, 0);
    mainWidget->setLayout(scrollAreaLayout);

    scrollArea[0] = new QScrollArea();
    imageLabel[0] = new ImageWidget(mainWindow->penColor, 2, scrollArea[0]);
    imageLabelContianer[0] = new QWidget();
    QVBoxLayout *imgLabelContianerLayout0 = new QVBoxLayout(imageLabelContianer[0]);
    imgLabelContianerLayout0->addWidget(imageLabel[0]);
    scrollArea[0]->setWidget(imageLabelContianer[0]);
    scrollArea[0]->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    scrollArea[0]->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea[0]->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea[0]->setFrameShape(QFrame::NoFrame);

    zoomRatioLabel = new QLabel("1.00x");
    zoomRatioLabel->setAlignment(Qt::AlignmentFlag::AlignHCenter);

    exchangeAreaPreviewBtn = new QPushButton();
    exchangeAreaPreviewBtn->setMaximumWidth(40);
    exchangeAreaPreviewBtn->setIcon(QIcon(":/image/resource/right2left.png"));
    exchangeAreaPreviewBtn->setStyleSheet("QPushButton{border: none}");
    exchangeAreaPreviewBtn->setIconSize(QSize(30, 16));

    imageInfoBtn = new QPushButton();
    imageInfoBtn->setIcon(QIcon(":/image/resource/info.svg"));
    imageInfoBtn->setMaximumWidth(40);
    imageInfoBtn->setStyleSheet("QPushButton{border: none} QPushButton:hover{background-color:#41a7e0} QPushButton:pressed{background-color:#a7a7a7}");

    QVBoxLayout *centerLayout = new QVBoxLayout();
    centerLayout->addStretch(1);
    centerLayout->addWidget(zoomRatioLabel);
    centerLayout->addWidget(exchangeAreaPreviewBtn);
    centerLayout->addWidget(imageInfoBtn);
    centerLayout->addStretch(1);

    scrollArea[1] = new QScrollArea();
    imageLabel[1] = new ImageWidget(mainWindow->penColor, 2, scrollArea[1]);
    imageLabelContianer[1] = new QWidget();
    QVBoxLayout *imgLabelContianerLayout1 = new QVBoxLayout(imageLabelContianer[1]);
    imgLabelContianerLayout1->addWidget(imageLabel[1]);
    scrollArea[1]->setWidget(imageLabelContianer[1]);
    scrollArea[1]->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    scrollArea[1]->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea[1]->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea[1]->setFrameShape(QFrame::NoFrame);

    scrollAreaLayout->addWidget(scrollArea[0], 1);
    scrollAreaLayout->addLayout(centerLayout, 0);
    scrollAreaLayout->addWidget(scrollArea[1], 1);

    mainWindow->setCentralWidget(mainWidget);

    createDockWidget0(mainWindow);

    createDockWidget1(mainWindow);
}

void Ui::IIPviewerUi::createDockWidget0(IIPviewer *mainWindow)
{
    dataAnalyseDockWgt = new QDockWidget(mainWindow);
    dataAnalyseDockWgt->setWindowTitle("data analyse");
    dataAnalyseDockWgt->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QWidget *toolBoxContainer = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout();
    toolBoxContainer->setLayout(vlayout);
    dataAnalyseDockWgt->setWidget(toolBoxContainer);
    QIntValidator *imageSizeValidator = new QIntValidator(0, 10000, dataAnalyseDockWgt);
    QGridLayout *paintPosLayout0 = new QGridLayout();
    QLabel *start_x_text = new QLabel("start x:");
    start_x_edit0 = new QLineEdit();
    start_x_edit0->setMaximumWidth(55);
    start_x_edit0->setValidator(imageSizeValidator);
    QLabel *start_y_text = new QLabel("y:");
    start_y_edit0 = new QLineEdit();
    start_y_edit0->setMaximumWidth(55);
    start_y_edit0->setValidator(imageSizeValidator);
    paintPosLayout0->addWidget(start_x_text, 0, 0, 1, 1);
    paintPosLayout0->addWidget(start_x_edit0, 0, 1, 1, 1);
    paintPosLayout0->addWidget(start_y_text, 1, 0, 1, 1);
    paintPosLayout0->addWidget(start_y_edit0, 1, 1, 1, 1);
    QLabel *end_x_text = new QLabel("end x:");
    end_x_edit0 = new QLineEdit();
    end_x_edit0->setMaximumWidth(55);
    end_x_edit0->setValidator(imageSizeValidator);
    QLabel *end_y_text = new QLabel("y:");
    end_y_edit0 = new QLineEdit();
    end_y_edit0->setMaximumWidth(55);
    end_y_edit0->setValidator(imageSizeValidator);
    paintPosLayout0->addWidget(end_x_text, 2, 0, 1, 1);
    paintPosLayout0->addWidget(end_x_edit0, 2, 1, 1, 1);
    paintPosLayout0->addWidget(end_y_text, 3, 0, 1, 1);
    paintPosLayout0->addWidget(end_y_edit0, 3, 1, 1, 1);
    paintOk0 = new QPushButton("ok");
    paintPosLayout0->addWidget(paintOk0, 4, 1, 1, 1);
    QFrame *vline1 = new QFrame();
    vline1->setFrameShape(QFrame::VLine);
    QVBoxLayout *syncLayout = new QVBoxLayout();
    syncRight = new QPushButton("--->");
    syncLeft = new QPushButton("<---");
    clearPaintBtn = new QPushButton("clear");
    syncLayout->addWidget(syncRight);
    syncLayout->addWidget(syncLeft);
    syncLayout->addWidget(clearPaintBtn);
    QFrame *vline2 = new QFrame();
    vline2->setFrameShape(QFrame::VLine);

    QGridLayout *paintPosLayout1 = new QGridLayout();
    QLabel *start_x_text1 = new QLabel("start x:");
    start_x_edit1 = new QLineEdit();
    start_x_edit1->setMaximumWidth(55);
    start_x_edit1->setValidator(imageSizeValidator);
    QLabel *start_y_text1 = new QLabel("y:");
    start_y_edit1 = new QLineEdit();
    start_y_edit1->setMaximumWidth(55);
    start_y_edit1->setValidator(imageSizeValidator);
    paintPosLayout1->addWidget(start_x_text1, 0, 0, 1, 1);
    paintPosLayout1->addWidget(start_x_edit1, 0, 1, 1, 1);
    paintPosLayout1->addWidget(start_y_text1, 1, 0, 1, 1);
    paintPosLayout1->addWidget(start_y_edit1, 1, 1, 1, 1);
    QLabel *end_x_text1 = new QLabel("end x:");
    end_x_edit1 = new QLineEdit();
    end_x_edit1->setMaximumWidth(55);
    end_x_edit1->setValidator(imageSizeValidator);
    QLabel *end_y_text1 = new QLabel("y:");
    end_y_edit1 = new QLineEdit();
    end_y_edit1->setMaximumWidth(55);
    end_y_edit1->setValidator(imageSizeValidator);
    paintPosLayout1->addWidget(end_x_text1, 2, 0, 1, 1);
    paintPosLayout1->addWidget(end_x_edit1, 2, 1, 1, 1);
    paintPosLayout1->addWidget(end_y_text1, 3, 0, 1, 1);
    paintPosLayout1->addWidget(end_y_edit1, 3, 1, 1, 1);

    paintOk1 = new QPushButton("ok");
    paintPosLayout1->addWidget(paintOk1, 4, 1, 1, 1);

    QHBoxLayout *paintPosLayout = new QHBoxLayout();
    paintPosLayout->addLayout(paintPosLayout0);
    paintPosLayout->addWidget(vline1);
    paintPosLayout->addLayout(syncLayout);
    paintPosLayout->addWidget(vline2);
    paintPosLayout->addLayout(paintPosLayout1);

    vlayout->addLayout(paintPosLayout);

    QFrame *h_line = new QFrame();
    h_line->setFrameShape(QFrame::HLine);
    vlayout->addWidget(h_line);

    QGridLayout *plot_btn_layout = new QGridLayout();
    plot_rgb_contourf_line = new QPushButton("rgb contour/line");
    plot_rgb_hist = new QPushButton("rgb hist");
    plot_yuv_contourf_line = new QPushButton("yuv contour/line");
    plot_yuv_hist = new QPushButton("yuv hist");
    plot_hsv_contourf_line = new QPushButton("hsv contour/line");
    plot_hsv_hist = new QPushButton("hsv hist");
    plot_btn_layout->addWidget(plot_rgb_contourf_line, 0, 0, 1, 1);
    plot_btn_layout->addWidget(plot_rgb_hist, 0, 1, 1, 1);
    plot_btn_layout->addWidget(plot_yuv_contourf_line, 1, 0, 1, 1);
    plot_btn_layout->addWidget(plot_yuv_hist, 1, 1, 1, 1);
    plot_btn_layout->addWidget(plot_hsv_contourf_line, 2, 0, 1, 1);
    plot_btn_layout->addWidget(plot_hsv_hist, 2, 1, 1, 1);

    vlayout->addLayout(plot_btn_layout);

    QGridLayout *rgb2yuv_mat_layout = new QGridLayout();
    QLabel *rgb2yuv_mat_label = new QLabel("rgb2yuv:");
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_label, 0, 0, 1, 1);
    QDoubleValidator *validtor = new QDoubleValidator(-5.0, 5.0, 10, dataAnalyseDockWgt);
    rgb2yuv_mat_0 = new QLineEdit("0.299");
    rgb2yuv_mat_0->setMaximumWidth(55);
    rgb2yuv_mat_0->setValidator(validtor);
    rgb2yuv_mat_1 = new QLineEdit("0.587");
    rgb2yuv_mat_1->setMaximumWidth(55);
    rgb2yuv_mat_1->setValidator(validtor);
    rgb2yuv_mat_2 = new QLineEdit("0.114");
    rgb2yuv_mat_2->setMaximumWidth(55);
    rgb2yuv_mat_2->setValidator(validtor);
    rgb2yuv_mat_3 = new QLineEdit("-0.147");
    rgb2yuv_mat_3->setMaximumWidth(55);
    rgb2yuv_mat_3->setValidator(validtor);
    rgb2yuv_mat_4 = new QLineEdit("-0.289");
    rgb2yuv_mat_4->setMaximumWidth(55);
    rgb2yuv_mat_4->setValidator(validtor);
    rgb2yuv_mat_5 = new QLineEdit("0.436");
    rgb2yuv_mat_5->setMaximumWidth(55);
    rgb2yuv_mat_5->setValidator(validtor);
    rgb2yuv_mat_6 = new QLineEdit("0.615");
    rgb2yuv_mat_6->setMaximumWidth(55);
    rgb2yuv_mat_6->setValidator(validtor);
    rgb2yuv_mat_7 = new QLineEdit("-0.515");
    rgb2yuv_mat_7->setMaximumWidth(55);
    rgb2yuv_mat_7->setValidator(validtor);
    rgb2yuv_mat_8 = new QLineEdit("-0.100");
    rgb2yuv_mat_8->setMaximumWidth(55);
    rgb2yuv_mat_8->setValidator(validtor);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_0, 1, 0, 1, 1);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_1, 1, 1, 1, 1);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_2, 1, 2, 1, 1);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_3, 2, 0, 1, 1);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_4, 2, 1, 1, 1);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_5, 2, 2, 1, 1);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_6, 3, 0, 1, 1);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_7, 3, 1, 1, 1);
    rgb2yuv_mat_layout->addWidget(rgb2yuv_mat_8, 3, 2, 1, 1);
    rgb2yuv_mat_layout->setColumnMinimumWidth(0, 20);
    rgb2yuv_mat_layout->setColumnMinimumWidth(1, 20);
    rgb2yuv_mat_layout->setColumnMinimumWidth(2, 20);

    vlayout->addLayout(rgb2yuv_mat_layout);
    vlayout->addStretch(1);

    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dataAnalyseDockWgt);
}

void Ui::IIPviewerUi::createDockWidget1(IIPviewer *mainWindow)
{
    playListDockWgt = new QDockWidget(mainWindow);
    playListDockWgt->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    playListDockWgt->setWindowTitle("play list");
    // playListDockWgt.setFeatures(QDockWidget.DockWidgetClosable |
    // QDockWidget.DockWidgetMovable | QDockWidget.DockWidgetFloatable |
    // QDockWidget.DockWidgetVerticalTitleBar)
    QWidget *pltWgtContainer = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout();
    pltWgtContainer->setLayout(vlayout);
    playListDockWgt->setWidget(pltWgtContainer);

    vlayout->addStretch(1);
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, playListDockWgt);
}
