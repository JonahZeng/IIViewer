#include "config.h"
#include "IIViewer.h"
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QBuffer>
#include <QImageReader>
#include <QInputDialog>
#include <QLocale>
#include <QProgressDialog>

namespace
{
constexpr qint64 SECONDS_PER_DAY = 24 * 60 * 60;
constexpr qint64 UPDATE_CHECK_INTERVAL_DAYS = 7; // 自动检查更新的间隔（天）
}

void IIViewer::checkUpdateIfDue()
{
    const QDateTime now = QDateTime::currentDateTime();
    const qint64 elapsedSeconds = settings.lastUpdateCheckTime.isValid() ? settings.lastUpdateCheckTime.secsTo(now) : -1;
    // 上次检查时间在有效期内则跳过；时间为空、已过期或系统时钟被回拨时重新检查
    if (elapsedSeconds >= 0 && elapsedSeconds < UPDATE_CHECK_INTERVAL_DAYS * SECONDS_PER_DAY)
    {
        return;
    }
    checkUpdate(true);
}

void IIViewer::checkUpdate(bool silent)
{
    QNetworkRequest request(QUrl("https://api.github.com/repos/JonahZeng/IIViewer/releases/latest"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(30'000);
#endif
    QNetworkReply *reply = networkManager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, silent] {
        bool checkedSuccessfully = false;
        QVersionNumber newVersion;
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray const response = reply->readAll();
            QJsonDocument const jsonDoc = QJsonDocument::fromJson(response);
            const QJsonObject jsonObj = jsonDoc.object();
            QString latestVersion = jsonObj.value("tag_name").toString();
            if (!latestVersion.isEmpty())
            {
                if (latestVersion.startsWith('v', Qt::CaseSensitivity::CaseInsensitive))
                {
                    latestVersion.remove(QChar('v'), Qt::CaseSensitivity::CaseInsensitive);
                }
                newVersion = QVersionNumber::fromString(latestVersion);
                checkedSuccessfully = !newVersion.isNull();
            }
        }

        if (checkedSuccessfully)
        {
            // 仅在成功获取版本号后记录时间，网络异常时下次启动会自动重试
            settings.lastUpdateCheckTime = QDateTime::currentDateTime();
            settings.dumpSettingsToFile();
        }

        const QVersionNumber currentVersion{IIViewer_VERSION_MAJOR, IIViewer_VERSION_MINOR, IIViewer_VERSION_PATCH}; // 当前版本号
        if (checkedSuccessfully && newVersion > currentVersion)
        {
            // 自动检查发现新版本时同样提示用户
            const QString text("New version found, <a href=\"https://github.com/JonahZeng/IIViewer/releases\">click here to download</a>");
            QMessageBox msgBox(QMessageBox::Icon::Information, tr("New version found"), text, QMessageBox::StandardButton::Ok, this);
            msgBox.exec();
        }
        else if (!silent)
        {
            if (checkedSuccessfully)
            {
                QMessageBox::information(this, tr("no new version"), tr("You are using the latest version"), QMessageBox::StandardButton::Ok);
            }
            else
            {
                QMessageBox::critical(this, tr("network error"), QString("Error checking for updates: %1").arg(reply->errorString()), QMessageBox::StandardButton::Ok);
            }
        }
        reply->deleteLater();
    });
}

bool IIViewer::isRemoteUrl(const QString &sulr)
{
    return sulr.startsWith("http://", Qt::CaseInsensitive) || sulr.startsWith("https://", Qt::CaseInsensitive);
}

void IIViewer::onOpenUrlAction()
{
    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("open image from url"),
        tr("Image URL:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || text.trimmed().isEmpty())
    {
        return;
    }

    const QUrl url = QUrl::fromUserInput(text.trimmed());
    if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https"))
    {
        QMessageBox::warning(this, tr("invalid url"),
            tr("Please input a valid http/https url"), QMessageBox::StandardButton::Ok);
        return;
    }

    if (sender() == static_cast<QObject *>(ui.openUrlRightAction))
    {
        loadNetworkImage(url, RIGHT_IMG_WIDGET);
    }
    else
    {
        loadNetworkImage(url, LEFT_IMG_WIDGET);
    }
}

void IIViewer::loadNetworkImage(const QUrl &url, int scrollArea)
{
    QNetworkRequest request(url);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(30'000); // 避免挂死的连接无限期占用 reply/socket
#endif
    QNetworkReply *reply = networkManager.get(request); // manager 为成员，连接池复用，随窗口析构

    // 前台下载进度提示：未知总大小时显示忙碌状态，已知时显示百分比
    auto *progressDlg = new QProgressDialog(tr("Downloading image..."), tr("Cancel"), 0, 0, this);
    progressDlg->setWindowTitle(tr("Download image"));
    progressDlg->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    progressDlg->setWindowModality(Qt::WindowModal);
    progressDlg->setMinimumDuration(300); // 快速下载不弹窗，避免闪烁
    progressDlg->setAutoClose(false);
    progressDlg->setAutoReset(false);
    progressDlg->setValue(0);

    QObject::connect(progressDlg, &QProgressDialog::canceled, reply, [reply]() { reply->abort(); });

    QObject::connect(reply, &QNetworkReply::downloadProgress, progressDlg,
        [this, progressDlg](qint64 received, qint64 total)
        {
            if (total <= 0)
            {
                progressDlg->setRange(0, 0); // 未知总大小，显示忙碌条
                progressDlg->setLabelText(tr("Downloading image..."));
                return;
            }
            progressDlg->setRange(0, 100);
            progressDlg->setValue(static_cast<int>(received * 100 / total));
            progressDlg->setLabelText(tr("Downloading image... %1 / %2")
                                          .arg(QLocale().formattedDataSize(received),
                                               QLocale().formattedDataSize(total)));
        });

    QObject::connect(reply, &QNetworkReply::finished, this,
        [this, reply, url, scrollArea, progressDlg]
        {
            progressDlg->close();
            progressDlg->deleteLater();

            const bool canceled = reply->error() == QNetworkReply::OperationCanceledError;
            if (reply->error() != QNetworkReply::NoError)
            {
                if (!canceled)
                {
                    QMessageBox::critical(this, tr("network error"),
                        tr("Failed to download image: %1").arg(reply->errorString()),
                        QMessageBox::StandardButton::Ok);
                }
                reply->deleteLater();
                return;
            }

            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (httpStatus >= 400)
            {
                QMessageBox::critical(this, tr("network error"),
                    tr("Failed to download image: HTTP %1").arg(httpStatus),
                    QMessageBox::StandardButton::Ok);
                reply->deleteLater();
                return;
            }

            QByteArray data = reply->readAll();
            reply->deleteLater();

            QBuffer buffer(&data);
            buffer.open(QIODevice::ReadOnly);
            QImageReader reader(&buffer);
            reader.setAutoTransform(true);
            reader.setAutoDetectImageFormat(true); // 按内容识别 jpg/webp，忽略 URL 后缀/token
            const QImage image = reader.read();
            if (image.isNull())
            {
                QMessageBox::critical(this, tr("error"),
                    tr("Failed to decode image: %1").arg(reader.errorString()),
                    QMessageBox::StandardButton::Ok);
                return;
            }

            const int other = (scrollArea == LEFT_IMG_WIDGET) ? RIGHT_IMG_WIDGET : LEFT_IMG_WIDGET;
            if (!openedFile.at(other).isEmpty() && image.size() != originSize.at(other))
            {
                QMessageBox::warning(this, tr("warning"), tr("image0 size != image1 size"),
                    QMessageBox::StandardButton::Ok);
                return;
            }

            const QString urlStr = url.toString();
            if (scrollArea == LEFT_IMG_WIDGET)
            {
                onCloseLeftFileAction();
                openedFile.at(0) = urlStr; // 标题栏显示 URL
                originSize.at(0) = image.size();
                ui.imageLabel.at(LEFT_IMG_WIDGET)->imgName = &openedFile.at(0);
                ui.imageLabel.at(LEFT_IMG_WIDGET)->setPixmap(image);
                loadFilePostProcessLayoutAndScrollValue(LEFT_IMG_WIDGET);
            }
            else
            {
                onCloseRightFileAction();
                openedFile.at(1) = urlStr;
                originSize.at(1) = image.size();
                ui.imageLabel.at(RIGHT_IMG_WIDGET)->imgName = &openedFile.at(1);
                ui.imageLabel.at(RIGHT_IMG_WIDGET)->setPixmap(image);
                loadFilePostProcessLayoutAndScrollValue(RIGHT_IMG_WIDGET);
            }

            masterScrollarea = ui.scrollArea.at(scrollArea);
            setTitle();
            emit updateExchangeBtnStatus();
            emit updateZoomLabelStatus();
        });
}
