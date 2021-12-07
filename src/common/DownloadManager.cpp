#include "DownloadManager.hpp"

#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"

#include <QDesktopServices>

namespace chatterino {

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent)
    , manager_(new QNetworkAccessManager)
{
}

DownloadManager::~DownloadManager()
{
    this->manager_->deleteLater();
}

void DownloadManager::setFile(QString fileURL, const QString &channelName)
{
    QString saveFilePath;
    saveFilePath =
        getPaths()->twitchProfileAvatars + "/twitch/" + channelName + ".png";
    QNetworkRequest request;
    request.setUrl(QUrl(fileURL));
    this->reply_ = this->manager_->get(request);

    this->file_ = new QFile;
    this->file_->setFileName(saveFilePath);
    this->file_->open(QIODevice::WriteOnly);

    connect(this->reply_, SIGNAL(downloadProgress(qint64, qint64)), this,
            SLOT(onDownloadProgress(qint64, qint64)));
    connect(this->manager_, SIGNAL(finished(QNetworkReply *)), this,
            SLOT(onFinished(QNetworkReply *)));
    connect(this->reply_, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(this->reply_, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void DownloadManager::onDownloadProgress(qint64 bytesRead, qint64 bytesTotal)
{
    qCDebug(chatterinoCommon)
        << "Download progress: " << bytesRead << "/" << bytesTotal;
}

void DownloadManager::onFinished(QNetworkReply *reply)
{
    switch (reply->error())
    {
        case QNetworkReply::NoError: {
            qCDebug(chatterinoCommon) << "file is downloaded successfully.";
        }
        break;
        default: {
            qCDebug(chatterinoCommon) << reply->errorString().toLatin1();
        };
    }

    if (this->file_->isOpen())
    {
        this->file_->close();
        this->file_->deleteLater();
    }
    emit downloadComplete();
}

void DownloadManager::onReadyRead()
{
    this->file_->write(this->reply_->readAll());
}

void DownloadManager::onReplyFinished()
{
    if (this->file_->isOpen())
    {
        this->file_->close();
        this->file_->deleteLater();
    }
}

}  // namespace chatterino
