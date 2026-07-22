#include "config.h"
#include "IIViewer.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QImageReader>
#include <libheif/heif.h>
#include <dng/dng_parser.h>

void IIViewer::loadYuvFile(const QString &fileName, int scrollArea, bool reload) // NOLINT(readability-function-cognitive-complexity)
{
    YuvFileInfoDlg dlg(this);

    auto yuvtp = settings.yuvType;
    if(yuvtp >= YuvType::YUV444_INTERLEAVE && yuvtp <= YuvType::YUV422_UYVY_AS1)
    {
        dlg.ui.formatComboBox->setCurrentIndex(static_cast<int>(yuvtp));
    }
    dlg.ui.bitDepthSpinBox->setValue(settings.yuv_bitDepth);
    dlg.ui.widthLineEdit->setText(QString::asprintf("%d", settings.yuv_width));
    dlg.ui.heightLineEdit->setText(QString::asprintf("%d", settings.yuv_height));

    if (dlg.exec() != YuvFileInfoDlg::Accepted)
    {
        return;
    }

    const int bitDepth = dlg.ui.bitDepthSpinBox->value();
    int pixSize = 2;
    if (bitDepth <= BIT8)
    {
        pixSize = 1;
    }
    else if (bitDepth > BIT8 && bitDepth <= BIT16)
    {
        pixSize = 2;
    }
    else
    {
        QMessageBox::warning(this, tr("error"), tr("yuv bits > 16"), QMessageBox::StandardButton::Ok);
        return;
    }

    const int width = dlg.ui.widthLineEdit->text().toInt();
    const int height = dlg.ui.heightLineEdit->text().toInt();
    qint64 totalSize = 0;
    YuvType yuv_tp = YuvType::YUV444_INTERLEAVE;
    const int curIdx = dlg.ui.formatComboBox->currentIndex();
    if (curIdx == YuvType::YUV444_INTERLEAVE)
    {
        yuv_tp = YuvType::YUV444_INTERLEAVE;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U;
    }
    else if (curIdx == YuvType::YUV444_PLANAR)
    {
        yuv_tp = YuvType::YUV444_PLANAR;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U;
    }
    else if (curIdx == YuvType::YUV422_UYVY)
    {
        yuv_tp = YuvType::YUV422_UYVY;
        totalSize = static_cast<qint64>(width) * height * pixSize * 2U;
    }
    else if (curIdx == YuvType::YUV422_YUYV)
    {
        yuv_tp = YuvType::YUV422_YUYV;
        totalSize = static_cast<qint64>(width) * height * pixSize * 2U;
    }
    else if (curIdx == YuvType::YUV420_NV12)
    {
        yuv_tp = YuvType::YUV420_NV12;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U / 2U;
    }
    else if (curIdx == YuvType::YUV420_NV21)
    {
        yuv_tp = YuvType::YUV420_NV21;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U / 2U;
    }
    else if (curIdx == YuvType::YUV420P_YU12)
    {
        yuv_tp = YuvType::YUV420P_YU12;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U / 2U;
    }
    else if (curIdx == YuvType::YUV420P_YV12)
    {
        yuv_tp = YuvType::YUV420P_YV12;
        totalSize = static_cast<qint64>(width) * height * pixSize * 3U / 2U;
    }
    else if (curIdx == YuvType::YUV400)
    {
        yuv_tp = YuvType::YUV400;
        totalSize = static_cast<qint64>(width) * height * pixSize;
    }
    else if (curIdx == YuvType::YUV422_YUYV_AS1)
    {
        yuv_tp = YuvType::YUV422_YUYV_AS1;
        totalSize = static_cast<qint64>(width) * height * pixSize * 2U;
    }
    else if (curIdx == YuvType::YUV422_UYVY_AS1)
    {
        yuv_tp = YuvType::YUV422_UYVY_AS1;
        totalSize = static_cast<qint64>(width) * height * pixSize * 2U;
    }

    settings.yuvType = yuv_tp;
    settings.yuv_bitDepth = bitDepth;
    settings.yuv_width = width;
    settings.yuv_height = height;

    const QFileInfo fileInfo(fileName);
    const qint64 yuvFileSize = fileInfo.size();
    if (totalSize > yuvFileSize)
    {
        QMessageBox::warning(this, tr("error"), tr("yuv file size < your require"), QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (QSize(width, height) != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        openedFile.at(0) = fileName;
        originSize.at(0) = QSize(width, height);
        setYuvImage(openedFile.at(0), yuv_tp, bitDepth, width, height, pixSize, LEFT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (!openedFile.at(0).isEmpty())
        {
            if (QSize(width, height) != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseRightFileAction();
        }
        openedFile.at(1) = fileName;
        originSize.at(1) = QSize(width, height);
        setYuvImage(openedFile.at(1), yuv_tp, bitDepth, width, height, pixSize, RIGHT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadRawFile(const QString &fileName, int scrollArea, bool reload) // NOLINT(readability-function-cognitive-complexity)
{
    RawFileInfoDlg dlg(this);
    switch (settings.rawByType)
    {
    case BayerPatternType::RGGB:
        dlg.ui.RGGBRadioButton->setChecked(true);
        break;
    case BayerPatternType::GRBG:
        dlg.ui.GRBGRadioButton->setChecked(true);
        break;
    case BayerPatternType::GBRG:
        dlg.ui.GBRGRadioButton->setChecked(true);
        break;
    case BayerPatternType::BGGR:
        dlg.ui.BGGRRadioButton->setChecked(true);
        break;
    case BayerPatternType::RGGIR:
        dlg.ui.RGGIRRadioButton->setChecked(true);
        break;
    case BayerPatternType::BGGIR:
        dlg.ui.BGGIRRadioButton->setChecked(true);
        break;
    case BayerPatternType::GRIRG:
        dlg.ui.GRIRGRadioButton->setChecked(true);
        break;
    case BayerPatternType::GBIRG:
        dlg.ui.GBIRGRadioButton->setChecked(true);
        break;
    case BayerPatternType::GIRRG:
        dlg.ui.GIRRGRadioButton->setChecked(true);
        break;
    case BayerPatternType::GIRBG:
        dlg.ui.GIRBGRadioButton->setChecked(true);
        break;
    case BayerPatternType::IRGGR:
        dlg.ui.IRGGRRadioButton->setChecked(true);
        break;
    case BayerPatternType::IRGGB:
        dlg.ui.IRGGBRadioButton->setChecked(true);
        break;
    case BayerPatternType::MONO:
        dlg.ui.MONORadioButton->setChecked(true);
        break;
    
    default:
        break;
    }
    
    auto byteOrder = settings.rawByteOrder;
    if (byteOrder == ByteOrderType::RAW_LITTLE_ENDIAN)
    {
        dlg.ui.little_endian->setChecked(true);
    }
    else if (byteOrder == ByteOrderType::RAW_BIG_ENDIAN)
    {
        dlg.ui.big_endian->setChecked(true);
    }
    if(!settings.raw_compact)
    {
        dlg.ui.BitDepthComboBox->setCurrentText(QString::asprintf("%d", settings.raw_bitDepth));
    }
    else
    {
        dlg.ui.BitDepthComboBox->setCurrentText(QString::asprintf("%d-comp", settings.raw_bitDepth));
    }

    dlg.ui.WidthLineEdit->setText(QString::asprintf("%d", settings.raw_width));
    dlg.ui.HeightLineEdit->setText(QString::asprintf("%d", settings.raw_height));

    if (dlg.exec() != RawFileInfoDlg::Accepted)
    {
        return;
    }
    BayerPatternType bay = BayerPatternType::BAYER_UNKNOW;
    if (dlg.ui.RGGBRadioButton->isChecked())
    {
        bay = BayerPatternType::RGGB;
    }
    else if (dlg.ui.GRBGRadioButton->isChecked())
    {
        bay = BayerPatternType::GRBG;
    }
    else if (dlg.ui.GBRGRadioButton->isChecked())
    {
        bay = BayerPatternType::GBRG;
    }
    else if (dlg.ui.BGGRRadioButton->isChecked())
    {
        bay = BayerPatternType::BGGR;
    }
    else if (dlg.ui.RGGIRRadioButton->isChecked())
    {
        bay = BayerPatternType::RGGIR;
    }
    else if (dlg.ui.BGGIRRadioButton->isChecked())
    {
        bay = BayerPatternType::BGGIR;
    }
    else if (dlg.ui.GRIRGRadioButton->isChecked())
    {
        bay = BayerPatternType::GRIRG;
    }
    else if (dlg.ui.GBIRGRadioButton->isChecked())
    {
        bay = BayerPatternType::GBIRG;
    }
    else if (dlg.ui.GIRRGRadioButton->isChecked())
    {
        bay = BayerPatternType::GIRRG;
    }
    else if (dlg.ui.GIRBGRadioButton->isChecked())
    {
        bay = BayerPatternType::GIRBG;
    }
    else if (dlg.ui.IRGGRRadioButton->isChecked())
    {
        bay = BayerPatternType::IRGGR;
    }
    else if (dlg.ui.IRGGBRadioButton->isChecked())
    {
        bay = BayerPatternType::IRGGB;
    }
    else if (dlg.ui.MONORadioButton->isChecked())
    {
        bay = BayerPatternType::MONO;
    }

    auto order = ByteOrderType::RAW_LITTLE_ENDIAN;
    if (dlg.ui.big_endian->isChecked())
    {
        order = ByteOrderType::RAW_BIG_ENDIAN;
    }
    bool isInt = false;
    const QString bit_option_str = dlg.ui.BitDepthComboBox->currentText();
    int bitDepth = bit_option_str.toInt(&isInt, 10); // NOLINT(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    bool raw_compact = false;
    if(!isInt && bit_option_str.endsWith(QString("-comp"))) // 12-comp, sep by '-'
    {
        auto bit_opt_list = bit_option_str.split(QChar('-'));
        bitDepth = bit_opt_list.at(0).toInt();
        raw_compact = true;
    }
    const int width = dlg.ui.WidthLineEdit->text().toInt();
    const int height = dlg.ui.HeightLineEdit->text().toInt();

    settings.rawByType = bay;
    settings.raw_bitDepth = bitDepth;
    settings.raw_width = width;
    settings.raw_height = height;
    settings.rawByteOrder = order;
    settings.raw_compact = raw_compact;

    int pixSize = 2;
    if (bitDepth <= BIT8)
    {
        pixSize = 1;
    }
    else if (bitDepth > BIT8 && bitDepth <= BIT16)
    {
        pixSize = 2;
    }
    else if (bitDepth > BIT16 && bitDepth <= BIT32)
    {
        pixSize = 4;
    }

    const QFileInfo fileInfo(fileName);
    const qint64 rawFileSize = fileInfo.size();
    if(raw_compact)
    {
        if(width * height * bitDepth / BIT8 > rawFileSize)
        {
            QMessageBox::information(this, tr("error"), tr("raw file size < your input"), QMessageBox::StandardButton::Ok);
            return;
        }
    }
    else if (static_cast<qint64>(pixSize) * width * height > rawFileSize)
    {
        QMessageBox::information(this, tr("error"), tr("raw file size < your input"), QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (QSize(width, height) != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        openedFile.at(0) = fileName;
        originSize.at(0) = QSize(width, height);
        setRawImage(openedFile.at(0), bay, order, bitDepth, raw_compact, width, height, LEFT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET); 
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (!openedFile.at(0).isEmpty())
        {
            if (QSize(width, height) != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseRightFileAction();
        }
        openedFile.at(1) = fileName;
        originSize.at(1) = QSize(width, height);
        setRawImage(openedFile.at(1), bay, order, bitDepth, raw_compact, width, height, RIGHT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadPnmFile(const QString &fileName, int scrollArea, bool reload)
{
    const QImageReader reader(fileName);
    if (!reader.canRead())
    {
        const QString txt_info = QString("can not open ") + fileName + QString(" as image!");
        QMessageBox::information(this, tr("error"), txt_info, QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (reader.size() != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        openedFile.at(0) = fileName;
        originSize.at(0) = reader.size();
        setImage(openedFile.at(0), LEFT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (openedFile.at(0).length() > 0)
        {
            if (reader.size() != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseRightFileAction();
        }
        openedFile.at(1) = fileName;
        originSize.at(1) = reader.size();
        setImage(openedFile.at(1), RIGHT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadPgmFile(const QString &fileName, int scrollArea, bool reload)
{
    const QImageReader reader(fileName);
    if (!reader.canRead())
    {
        const QString txt_info = QString("can not open ") + fileName + QString(" as image!");
        QMessageBox::information(this, tr("error"), txt_info, QMessageBox::StandardButton::Ok);
        return;
    }
    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (reader.size() != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        openedFile.at(0) = fileName;
        originSize.at(0) = reader.size();
        setImage(openedFile.at(0), LEFT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if (openedFile.at(0).length() > 0)
        {
            if (reader.size() != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                return;
            }
        }
        if(!reload)
        {
            onCloseRightFileAction();
        }
        openedFile.at(1) = fileName;
        originSize.at(1) = reader.size();
        setImage(openedFile.at(1), RIGHT_IMG_WIDGET); // 使用openedFile，因为imagewidget会存储它的指针
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadHeifFile(const QString &fileName, int scrollArea, bool reload) // NOLINT(readability-function-cognitive-complexity)
{
    // Load HEIC image using libheif
    heif_context* ctx = heif_context_alloc();
    if (ctx != nullptr)
    {
        const QString t_info = QString("Failed to allocate heif context for ") + fileName;
        QMessageBox::information(this, tr("error"), t_info, QMessageBox::StandardButton::Ok);
        return;
    }

    heif_error error = heif_context_read_from_file(ctx, fileName.toUtf8().constData(), nullptr);
    if (error.code != heif_error_Ok)
    {
        const QString t_info = QString("Failed to read HEIC file: ") + fileName + QString("\nError: ") + error.message;
        QMessageBox::information(this, tr("error"), t_info, QMessageBox::StandardButton::Ok);
        heif_context_free(ctx);
        return;
    }

    heif_image_handle* handle = nullptr; // NOLINT(misc-const-correctness)
    error = heif_context_get_primary_image_handle(ctx, &handle);
    if (error.code != heif_error_Ok)
    {
        const QString t_info = QString("Failed to get primary image handle from ") + fileName;
        QMessageBox::information(this, tr("error"), t_info, QMessageBox::StandardButton::Ok);
        heif_context_free(ctx);
        return;
    }

    const int width = heif_image_handle_get_width(handle);
    const int height = heif_image_handle_get_height(handle);

    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if (!openedFile.at(1).isEmpty())
        {
            if (QSize(width, height) != originSize.at(1))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                heif_image_handle_release(handle);
                heif_context_free(ctx);
                return;
            }
        }
    }
    else if(scrollArea == RIGHT_IMG_WIDGET)
    {
        if (openedFile.at(0).length() > 0)
        {
            if (QSize(width, height) != originSize.at(0))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                heif_image_handle_release(handle);
                heif_context_free(ctx);
                return;
            }
        }
    }

    const int l_bit_depth = heif_image_handle_get_luma_bits_per_pixel(handle);
    const int c_bit_depth = heif_image_handle_get_chroma_bits_per_pixel(handle);
    heif_colorspace csp = heif_colorspace::heif_colorspace_undefined;
    heif_chroma cch = heif_chroma::heif_chroma_undefined;
    heif_image_handle_get_preferred_decoding_colorspace(handle, &csp, &cch);
    if (l_bit_depth > BIT8 || c_bit_depth > BIT8 || csp != heif_colorspace::heif_colorspace_YCbCr || \
        (cch != heif_chroma::heif_chroma_420 && cch != heif_chroma::heif_chroma_422 && cch != heif_chroma::heif_chroma_444))
    {
        const QString txt_info = QString("Failed to decode HEIC image: ") + fileName + QString("\nError: ") + QString("not 8bit yuv image");
        QMessageBox::information(this, tr("error"), txt_info, QMessageBox::StandardButton::Ok);
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return;
    }

    if (scrollArea == LEFT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseLeftFileAction();
        }
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        openedFile.at(0) = fileName;
        originSize.at(0) = QSize(width, height);
        setImage(openedFile.at(0), LEFT_IMG_WIDGET);
        if(!reload)
        { 
            loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
    else if (scrollArea == RIGHT_IMG_WIDGET)
    {
        if(!reload)
        {
            onCloseRightFileAction();
        }
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        openedFile.at(1) = fileName;
        originSize.at(1) = QSize(width, height);
        setImage(openedFile.at(1), RIGHT_IMG_WIDGET);
        if(!reload) { 
            loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            addFileToHistory(fileName);
        }
    }
}

void IIViewer::loadDngFile(const QString &fileName, int scrollArea, bool reload) // NOLINT(readability-function-cognitive-complexity)
{
    const dng::DngImage dngImg = dng::DngParser::parse(fileName.toStdString());
    const int bitDepth = dngImg.bits_per_sample;
    const int width = dngImg.width;
    const int height = dngImg.height;
    const bool raw_compact = false;
    const ByteOrderType order = ByteOrderType::RAW_LITTLE_ENDIAN;
    BayerPatternType bay = BayerPatternType::BAYER_UNKNOW;
    // dngImg.samples_per_pixel;
    if(dngImg.photometric_interpretation == 32803) // CFA
    {
        if (dngImg.cfa_layout != 1 || dngImg.cfa_repeat_cols != 2 || dngImg.cfa_repeat_rows !=2)  // not rectangel layout bayer
        {
            QMessageBox::warning(this, tr("warning"), tr("not a 2x2 bayer raw!"), QMessageBox::StandardButton::Ok);
            return;
        }
        if(dngImg.cfa_pattern[0] == 0 && dngImg.cfa_pattern[1] == 1 && dngImg.cfa_pattern[2] == 1 && dngImg.cfa_pattern[3] == 2)
        {
            bay = BayerPatternType::RGGB;
        }
        else if(dngImg.cfa_pattern[0] == 1 && dngImg.cfa_pattern[1] == 0 && dngImg.cfa_pattern[2] == 2 && dngImg.cfa_pattern[3] == 1)
        {
            bay = BayerPatternType::GRBG;
        }
        else if(dngImg.cfa_pattern[0] == 1 && dngImg.cfa_pattern[1] == 2 && dngImg.cfa_pattern[2] == 0 && dngImg.cfa_pattern[3] == 1)
        {
            bay = BayerPatternType::GBRG;
        }
        else if(dngImg.cfa_pattern[0] == 2 && dngImg.cfa_pattern[1] == 1 && dngImg.cfa_pattern[2] == 1 && dngImg.cfa_pattern[3] == 0)
        {
            bay = BayerPatternType::BGGR;
        }
        else
        {
            QMessageBox::warning(this, tr("warning"), tr("bayer not one of RGGB/GRBG/GBRG/BGGR !"), QMessageBox::StandardButton::Ok);
            return;
        }
        if (scrollArea == LEFT_IMG_WIDGET)
        {
            if (!openedFile.at(1).isEmpty())
            {
                if (QSize(width, height) != originSize.at(1))
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            if (!reload)
            {
                onCloseLeftFileAction();
            }
            openedFile.at(0) = fileName;
            originSize.at(0) = QSize(width, height);
            setDngRawImage(openedFile.at(0), bay, order, bitDepth, raw_compact, width, height, LEFT_IMG_WIDGET, dngImg.raw_pixels.data());
            if (!reload)
            {
                loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
                addFileToHistory(fileName);
            }
        }
        else if (scrollArea == RIGHT_IMG_WIDGET)
        {
            if (!openedFile.at(0).isEmpty())
            {
                if (QSize(width, height) != originSize.at(0))
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            if (!reload)
            {
                onCloseRightFileAction();
            }
            openedFile.at(1) = fileName;
            originSize.at(1) = QSize(width, height);
            setDngRawImage(openedFile.at(1), bay, order, bitDepth, raw_compact, width, height, RIGHT_IMG_WIDGET, dngImg.raw_pixels.data());
            if (!reload)
            {
                loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
                addFileToHistory(fileName);
            }
        }
    }
    else if(dngImg.photometric_interpretation == 34892) // LinearRaw
    {
        if(dngImg.samples_per_pixel != 3)
        {
            QMessageBox::warning(this, tr("warning"), tr("samples_per_pixel != 3"), QMessageBox::StandardButton::Ok);
            return;
        }
        if (scrollArea == LEFT_IMG_WIDGET)
        {
            if (!openedFile.at(1).isEmpty())
            {
                if (QSize(width, height) != originSize.at(1))
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            if (!reload)
            {
                onCloseLeftFileAction();
            }
            openedFile.at(0) = fileName;
            originSize.at(0) = QSize(width, height);
            setDngPnmImage(openedFile.at(0), bitDepth, width, height, LEFT_IMG_WIDGET, dngImg.raw_pixels.data());
            if (!reload)
            {
                loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
                addFileToHistory(fileName);
            }
        }
        else if (scrollArea == RIGHT_IMG_WIDGET)
        {
            if (openedFile.at(0).length() > 0)
            {
                if (QSize(width, height) != originSize.at(0))
                {
                    QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"), QMessageBox::StandardButton::Ok);
                    return;
                }
            }
            if (!reload)
            {
                onCloseRightFileAction();
            }
            openedFile.at(1) = fileName;
            originSize.at(1) = QSize(width, height);
            setDngPnmImage(openedFile.at(1), bitDepth, width, height, RIGHT_IMG_WIDGET, dngImg.raw_pixels.data());
            if (!reload)
            {
                loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
                addFileToHistory(fileName);
            }
        }
    }
}