#pragma once

#include "Application.hpp"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QStringList>

namespace chatterino {

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);
    virtual ~DownloadManager();
    void setFile(QString fileURL, const QString &channelName);

private:
    QNetworkAccessManager *manager;
    QNetworkReply *reply;
    QFile *file;

private slots:
    void onDownloadProgress(qint64, qint64);
    void onFinished(QNetworkReply *);
    void onReadyRead();
    void onReplyFinished();

signals:
    void downloadComplete();
};
}  // namespace chatterino
