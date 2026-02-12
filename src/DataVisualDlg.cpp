#include "DataVisualDlg.h"
#include <QGroupBox>
#include <QtDataVisualization/QValue3DAxis>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using QtDataVisualization::QSurfaceDataArray;
// using QtDataVisualization::QSurfaceDataItem;
using QtDataVisualization::QSurfaceDataProxy;
using QtDataVisualization::QSurfaceDataRow;
using QtDataVisualization::QValue3DAxis;
#endif

constexpr int BIT8 = 8;
constexpr int BIT16 = 16;
constexpr int BIT32 = 32;

DataVisualDialog::DataVisualDialog(QWidget *parent, bool prepared, ImageWidget *wgt, QPoint st0, QPoint st1) // NOLINT(readability-function-cognitive-complexity)
    : QDialog(parent), graph(new Q3DSurface())
{
    QWidget *container = QWidget::createWindowContainer(graph, this);

    graph->setAxisX(new QValue3DAxis);
    graph->setAxisY(new QValue3DAxis);
    graph->setAxisZ(new QValue3DAxis);

    graph->axisX()->setTitle("x");
    graph->axisZ()->setTitle("y");
    graph->axisY()->setTitle(tr("pixel value"));

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    QGroupBox *rgbGroup = new QGroupBox(tr("selection mode"));
    QHBoxLayout *rgbGroupLayout = new QHBoxLayout(rgbGroup);
    dispR = new QCheckBox("R");
    dispG = new QCheckBox("G");
    dispB = new QCheckBox("B");
    rgbGroupLayout->addWidget(dispR);
    rgbGroupLayout->addWidget(dispG);
    rgbGroupLayout->addWidget(dispB);
    // rgbGroup->setLayout(rgbGroupLayout);

    selectNoItem = new QRadioButton(tr("none"));
    selectNoItem->setChecked(true);
    selectItem = new QRadioButton(tr("item"));
    selectItem->setChecked(false);
    selectItemAndRow = new QRadioButton(tr("itemAndRow"));
    selectItemAndRow->setChecked(false);
    selectItemAndCol = new QRadioButton(tr("itemAndCol"));
    selectItemAndCol->setChecked(false);
    selectionGroup.addButton(selectNoItem, 1);
    selectionGroup.addButton(selectItem, 2);
    selectionGroup.addButton(selectItemAndRow, 3);
    selectionGroup.addButton(selectItemAndCol, 4);

    okBtn = new QPushButton(tr("OK"));
    cancelBtn = new QPushButton(tr("Cancel"));
    verticalLayout->addWidget(container);
    horizontalLayout->addWidget(rgbGroup);
    horizontalLayout->addWidget(selectNoItem);
    horizontalLayout->addWidget(selectItem);
    horizontalLayout->addWidget(selectItemAndRow);
    horizontalLayout->addWidget(selectItemAndCol);

    horizontalLayout->addStretch(1);
    horizontalLayout->addWidget(okBtn);
    horizontalLayout->addWidget(cancelBtn);
    verticalLayout->addLayout(horizontalLayout);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    verticalLayout->setMargin(8); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
#else
    verticalLayout->setContentsMargins(8, 8, 8, 8); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
#endif
    // setLayout(verticalLayout);
    resize(1024, 720); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    setWindowTitle(tr("Data Visual"));

    QSurfaceDataProxy *seriesProxy0 = new QSurfaceDataProxy();
    series0 = new QSurface3DSeries(seriesProxy0);
    QSurfaceDataProxy *seriesProxy1 = new QSurfaceDataProxy();
    series1 = new QSurface3DSeries(seriesProxy1);
    QSurfaceDataProxy *seriesProxy2 = new QSurfaceDataProxy();
    series2 = new QSurface3DSeries(seriesProxy2);

    series0->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
    series0->setFlatShadingEnabled(true);
    series1->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
    series1->setFlatShadingEnabled(true);
    series2->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
    series2->setFlatShadingEnabled(true);

    const int leftPos = st0.x() < st1.x() ? st0.x() : st1.x();
    const int width = std::abs(st0.x() - st1.x());
    const int topPos = st0.y() < st1.y() ? st0.y() : st1.y();
    const int height = std::abs(st0.y() - st1.y());
    int bitPerPixel = BIT8;
    if (prepared)
    {
        if (wgt->openedImgType == OpenedImageType::NORMAL_IMG)
        {
            QSurfaceDataArray *dataArray0 = new QSurfaceDataArray;
            QSurfaceDataArray *dataArray1 = new QSurfaceDataArray;
            QSurfaceDataArray *dataArray2 = new QSurfaceDataArray;
            for (int i = 0; i < height; i++)
            {
                QSurfaceDataRow *newRow0 = new QSurfaceDataRow(width);
                QSurfaceDataRow *newRow1 = new QSurfaceDataRow(width);
                QSurfaceDataRow *newRow2 = new QSurfaceDataRow(width);
                for (int j = 0; j < width; j++)
                {
                    wgt->pixMap->pixelColor((leftPos + j), (i + topPos)).red();
                    (*newRow0)[j].setPosition(QVector3D((float)j, (float)wgt->pixMap->pixelColor((leftPos + j), (i + topPos)).red(), (float)i));
                    (*newRow1)[j].setPosition(QVector3D((float)j, (float)wgt->pixMap->pixelColor((leftPos + j), (i + topPos)).green(), (float)i));
                    (*newRow2)[j].setPosition(QVector3D((float)j, (float)wgt->pixMap->pixelColor((leftPos + j), (i + topPos)).blue(), (float)i));
                }
                dataArray0->append(newRow0);
                dataArray1->append(newRow1);
                dataArray2->append(newRow2);
            }

            seriesProxy0->resetArray(dataArray0);
            seriesProxy1->resetArray(dataArray1);
            seriesProxy2->resetArray(dataArray2);

            series0->setBaseColor(QColor(255, 0, 0)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            series1->setBaseColor(QColor(0, 255, 0)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            series2->setBaseColor(QColor(0, 0, 255)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

            series0->setVisible(true);
            series1->setVisible(true);
            series2->setVisible(true);
            dispR->setChecked(true);
            dispG->setChecked(true);
            dispB->setChecked(true);

            graph->addSeries(series0);
            graph->addSeries(series1);
            graph->addSeries(series2);
        }
        else if (wgt->openedImgType == OpenedImageType::PNM_IMG)
        {
            bitPerPixel = wgt->pnmDataBit;
            uchar *buffer = wgt->pnmDataPtr;
            const int lineWidth = wgt->pixMap->width();
            QSurfaceDataArray *dataArray0 = new QSurfaceDataArray;
            QSurfaceDataArray *dataArray1 = new QSurfaceDataArray;
            QSurfaceDataArray *dataArray2 = new QSurfaceDataArray;
            for (int i = 0; i < height; i++)
            {
                QSurfaceDataRow *newRow0 = new QSurfaceDataRow(width);
                QSurfaceDataRow *newRow1 = new QSurfaceDataRow(width);
                QSurfaceDataRow *newRow2 = new QSurfaceDataRow(width);
                for (int j = 0; j < width; j++)
                {
                    const int pix_r = bitPerPixel <= 8 ? buffer[((i + topPos) * lineWidth * 3) + ((leftPos + j) * 3) + 0] : ((uint16_t *)buffer)[((i + topPos) * lineWidth * 3) + ((leftPos + j) * 3) + 0];
                    const int pix_g = bitPerPixel <= 8 ? buffer[((i + topPos) * lineWidth * 3) + ((leftPos + j) * 3) + 1] : ((uint16_t *)buffer)[((i + topPos) * lineWidth * 3) + ((leftPos + j) * 3) + 1];
                    const int pix_b = bitPerPixel <= 8 ? buffer[((i + topPos) * lineWidth * 3) + ((leftPos + j) * 3) + 2] : ((uint16_t *)buffer)[((i + topPos) * lineWidth * 3) + ((leftPos + j) * 3) + 2];
                    (*newRow0)[j].setPosition(QVector3D((float)j, (float)pix_r, (float)i));
                    (*newRow1)[j].setPosition(QVector3D((float)j, (float)pix_g, (float)i));
                    (*newRow2)[j].setPosition(QVector3D((float)j, (float)pix_b, (float)i));
                }
                dataArray0->append(newRow0);
                dataArray1->append(newRow1);
                dataArray2->append(newRow2);
            }
            seriesProxy0->resetArray(dataArray0);
            seriesProxy1->resetArray(dataArray1);
            seriesProxy2->resetArray(dataArray2);

            series0->setBaseColor(QColor(255, 0, 0)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            series1->setBaseColor(QColor(0, 255, 0)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            series2->setBaseColor(QColor(0, 0, 255)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            series0->setVisible(true);
            series1->setVisible(true);
            series2->setVisible(true);
            dispR->setChecked(true);
            dispG->setChecked(true);
            dispB->setChecked(true);
            graph->addSeries(series0);
            graph->addSeries(series1);
            graph->addSeries(series2);
        }
        else if(wgt->openedImgType == OpenedImageType::PGM_IMG)
        {
            bitPerPixel = wgt->pgmDataBit;
            uchar *buffer = wgt->pgmDataPtr;
            const int lineWidth = wgt->pixMap->width();
            QSurfaceDataArray *dataArray0 = new QSurfaceDataArray;
            for (int i = 0; i < height; i++)
            {
                QSurfaceDataRow *newRow0 = new QSurfaceDataRow(width);
                for (int j = 0; j < width; j++)
                {
                    const int pix_gray = bitPerPixel <= 8 ? buffer[((i + topPos) * lineWidth) + (leftPos + j)] : ((uint16_t *)buffer)[((i + topPos) * lineWidth) + (leftPos + j)];
                    (*newRow0)[j].setPosition(QVector3D((float)j, (float)pix_gray, (float)i));
                }
                dataArray0->append(newRow0);
            }
            seriesProxy0->resetArray(dataArray0);

            series0->setBaseColor(QColor(100, 100, 100)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            series0->setVisible(true);

            dispR->setChecked(true);
            dispG->setChecked(false);
            dispB->setChecked(false);
            graph->addSeries(series0);
            dispG->setDisabled(true);
            dispB->setDisabled(true);
        }
        else if (wgt->openedImgType == OpenedImageType::RAW_IMG)
        {
            bitPerPixel = wgt->rawDataBit;
            uchar *buffer = wgt->rawDataPtr;
            const int lineWidth = wgt->pixMap->width();
            QSurfaceDataArray *dataArray0 = new QSurfaceDataArray;
            for (int i = 0; i < height; i++)
            {
                QSurfaceDataRow *newRow0 = new QSurfaceDataRow(width);
                for (int j = 0; j < width; j++)
                {
                    int pix_val = 0;
                    if (bitPerPixel <= BIT8)
                    {
                        pix_val = buffer[((i + topPos) * lineWidth) + (leftPos + j)];
                    }
                    else if (bitPerPixel <= BIT16)
                    {
                        pix_val = ((uint16_t *)buffer)[((i + topPos) * lineWidth) + (leftPos + j)];
                    }
                    else if (bitPerPixel <= BIT32)
                    {
                        pix_val = (int32_t)((uint32_t *)buffer)[((i + topPos) * lineWidth) + (leftPos + j)];
                    }
                    (*newRow0)[j].setPosition(QVector3D((float)j, (float)pix_val, (float)i));
                }
                dataArray0->append(newRow0);
            }
            seriesProxy0->resetArray(dataArray0);
            series0->setBaseColor(QColor(160, 160, 160)); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
            series0->setVisible(true);
            // series1->setVisible(false);
            // series2->setVisible(false);
            dispR->setChecked(true);
            dispG->setChecked(false);
            dispB->setChecked(false);
            graph->addSeries(series0);
            // graph->addSeries(series1);
            // graph->addSeries(series2);
            dispG->setDisabled(true);
            dispB->setDisabled(true);
        }
        else if (wgt->openedImgType == OpenedImageType::YUV_IMG)
        {
            // bitPerPixel = wg->yuvDataBit;

            series0->setVisible(true);
            series1->setVisible(true);
            series2->setVisible(true);
            dispR->setChecked(true);
            dispG->setChecked(true);
            dispB->setChecked(true);
        }
    }
    graph->axisX()->setLabelFormat("%.2f");
    graph->axisZ()->setLabelFormat("%.2f");
    graph->axisX()->setAutoAdjustRange(true);
    graph->axisY()->setAutoAdjustRange(true);
    graph->axisZ()->setReversed(true);
    graph->axisZ()->setAutoAdjustRange(true);
    graph->axisX()->setLabelAutoRotation(30); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    graph->axisY()->setLabelAutoRotation(90); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    graph->axisZ()->setLabelAutoRotation(30); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

    graph->setSelectionMode(Q3DSurface::SelectionNone);

    connect(okBtn, &QPushButton::clicked, this, &DataVisualDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &DataVisualDialog::reject);
    connect(dispR, &QCheckBox::clicked, this, [this](bool checked)
            { this->series0->setVisible(checked); });
    connect(dispG, &QCheckBox::clicked, this, [this](bool checked)
            { this->series1->setVisible(checked); });
    connect(dispB, &QCheckBox::clicked, this, [this](bool checked)
            { this->series2->setVisible(checked); });

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&selectionGroup, &QButtonGroup::idClicked, this, [this](int idx)
    {
        if (idx == 1)
        {
            this->graph->setSelectionMode(Q3DSurface::SelectionNone);
        }
        else if (idx == 2)
        {
            this->graph->setSelectionMode(Q3DSurface::SelectionItem);
        }
        else if (idx == 3)
        {
            this->graph->setSelectionMode(Q3DSurface::SelectionItemAndRow | Q3DSurface::SelectionSlice | Q3DSurface::SelectionMultiSeries);
        }
        else if (idx == 4)
        {
            this->graph->setSelectionMode(Q3DSurface::SelectionItemAndColumn | Q3DSurface::SelectionSlice | Q3DSurface::SelectionMultiSeries);
        }
    });
#else
    connect(&selectionGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            [=](QAbstractButton *button)
            {
                if (button == selectNoItem)
                    this->graph->setSelectionMode(Q3DSurface::SelectionNone);
                else if (button == selectItem)
                    this->graph->setSelectionMode(Q3DSurface::SelectionItem);
                else if (button == selectItemAndRow)
                    this->graph->setSelectionMode(Q3DSurface::SelectionItemAndRow | Q3DSurface::SelectionSlice | Q3DSurface::SelectionMultiSeries);
                else if (button == selectItemAndCol)
                    this->graph->setSelectionMode(Q3DSurface::SelectionItemAndColumn | Q3DSurface::SelectionSlice | Q3DSurface::SelectionMultiSeries);
            });
#endif
}

DataVisualDialog::~DataVisualDialog()
{
    // destroy();
    delete graph;
}
