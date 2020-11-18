#include "DownloadManager.hpp"

#include "singletons/Paths.hpp"

#include <QDesktopServices>
#include "qlogging.hpp"

namespace chatterino {

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent)
{
    manager = new QNetworkAccessManager;
}

DownloadManager::~DownloadManager()
{
    manager->deleteLater();
}

void DownloadManager::setFile(QString fileURL, const QString &channelName)
{
    QString saveFilePath;
    saveFilePath =
        getPaths()->twitchProfileAvatars + "/twitch/" + channelName + ".png";
    QNetworkRequest request;
    request.setUrl(QUrl(fileURL));
    reply = manager->get(request);

    file = new QFile;
    file->setFileName(saveFilePath);
    file->open(QIODevice::WriteOnly);

    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this,
            SLOT(onDownloadProgress(qint64, qint64)));
    connect(manager, SIGNAL(finished(QNetworkReply *)), this,
            SLOT(onFinished(QNetworkReply *)));
    connect(reply, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
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

    if (file->isOpen())
    {
        file->close();
        file->deleteLater();
    }
    emit downloadComplete();
}

void DownloadManager::onReadyRead()
{
    file->write(reply->readAll());
}

void DownloadManager::onReplyFinished()
{
    if (file->isOpen())
    {
        file->close();
        file->deleteLater();
    }
}
}  // namespace chatterino
