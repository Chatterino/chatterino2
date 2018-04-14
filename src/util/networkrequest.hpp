#pragma once

#include "singletons/pathmanager.hpp"
#include "util/networkmanager.hpp"
#include "util/networkrequester.hpp"
#include "util/networkworker.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <QCryptographicHash>
#include <QFile>

namespace chatterino {
namespace util {

static QJsonObject parseJSONFromData(const QByteArray &data)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

    if (jsonDoc.isNull()) {
        return QJsonObject();
    }

    return jsonDoc.object();
}

static rapidjson::Document parseJSONFromData2(const QByteArray &data)
{
    rapidjson::Document ret(rapidjson::kNullType);

    rapidjson::ParseResult result = ret.Parse(data.data(), data.length());

    if (result.Code() != rapidjson::kParseErrorNone) {
        debug::Log("JSON parse error: {} ({})", rapidjson::GetParseError_En(result.Code()),
                   result.Offset());
        return ret;
    }

    return ret;
}

static rapidjson::Document parseJSONFromReply2(QNetworkReply *reply)
{
    rapidjson::Document ret(rapidjson::kNullType);

    if (reply->error() != QNetworkReply::NetworkError::NoError) {
        return ret;
    }

    QByteArray data = reply->readAll();
    rapidjson::ParseResult result = ret.Parse(data.data(), data.length());

    if (result.Code() != rapidjson::kParseErrorNone) {
        debug::Log("JSON parse error: {} ({})", rapidjson::GetParseError_En(result.Code()),
                   result.Offset());
        return ret;
    }

    return ret;
}

class NetworkRequest
{
    struct Data {
        QNetworkRequest request;
        const QObject *caller = nullptr;
        std::function<void(QNetworkReply *)> onReplyCreated;
        int timeoutMS = -1;
        bool useQuickLoadCache = false;

        QString getHash()
        {
            if (this->hash.isEmpty()) {
                QByteArray bytes;

                bytes.append(this->request.url().toString());

                for (const auto &header : this->request.rawHeaderList()) {
                    bytes.append(header);
                }

                QByteArray hashBytes(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256));

                this->hash = hashBytes.toHex();
            }

            return this->hash;
        }

        void writeToCache(const QByteArray &bytes);

    private:
        QString hash;
    } data;

public:
    NetworkRequest() = delete;
    explicit NetworkRequest(const char *url);
    explicit NetworkRequest(const std::string &url);
    explicit NetworkRequest(const QString &url);

    void setUseQuickLoadCache(bool value);

    void setCaller(const QObject *_caller)
    {
        this->data.caller = _caller;
    }

    void setOnReplyCreated(std::function<void(QNetworkReply *)> f)
    {
        this->data.onReplyCreated = f;
    }

    void setRawHeader(const QByteArray &headerName, const QByteArray &value)
    {
        this->data.request.setRawHeader(headerName, value);
    }

    void setTimeout(int ms)
    {
        this->data.timeoutMS = ms;
    }

    template <typename FinishedCallback>
    void get(FinishedCallback onFinished)
    {
        if (this->data.useQuickLoadCache) {
            auto &pathManager = singletons::PathManager::getInstance();

            QFile cachedFile(pathManager.cacheFolderPath + "/" + this->data.getHash());

            if (cachedFile.exists()) {
                if (cachedFile.open(QIODevice::ReadOnly)) {
                    QByteArray bytes = cachedFile.readAll();

                    // qDebug() << "Loaded cached resource" << this->data.request.url();

                    onFinished(bytes);

                    cachedFile.close();
                }
            }
        }

        QTimer *timer = nullptr;
        if (this->data.timeoutMS > 0) {
            timer = new QTimer;
        }

        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);

        if (this->data.caller != nullptr) {
            QObject::connect(worker, &NetworkWorker::doneUrl, this->data.caller,
                             [onFinished, data = this->data](auto reply) mutable {
                                 if (reply->error() != QNetworkReply::NetworkError::NoError) {
                                     // TODO: We might want to call an onError callback here
                                     return;
                                 }

                                 QByteArray bytes = reply->readAll();
                                 data.writeToCache(bytes);
                                 onFinished(bytes);

                                 reply->deleteLater();
                             });
        }

        if (timer != nullptr) {
            timer->start(this->data.timeoutMS);
        }

        QObject::connect(
            &requester, &NetworkRequester::requestUrl, worker,
            [timer, data = std::move(this->data), worker, onFinished{std::move(onFinished)}]() {
                QNetworkReply *reply = NetworkManager::NaM.get(data.request);

                if (timer != nullptr) {
                    QObject::connect(timer, &QTimer::timeout, worker, [reply, timer]() {
                        debug::Log("Aborted!");
                        reply->abort();
                        timer->deleteLater();
                    });
                }

                if (data.onReplyCreated) {
                    data.onReplyCreated(reply);
                }

                QObject::connect(reply, &QNetworkReply::finished, worker,
                                 [data = std::move(data), worker, reply,
                                  onFinished = std::move(onFinished)]() mutable {
                                     if (data.caller == nullptr) {
                                         QByteArray bytes = reply->readAll();
                                         data.writeToCache(bytes);
                                         onFinished(bytes);

                                         reply->deleteLater();
                                     } else {
                                         emit worker->doneUrl(reply);
                                     }

                                     delete worker;
                                 });
            });

        emit requester.requestUrl();
    }

    template <typename FinishedCallback>
    void getJSON(FinishedCallback onFinished)
    {
        this->get([onFinished{std::move(onFinished)}](const QByteArray &bytes) {
            auto object = parseJSONFromData(bytes);
            onFinished(object);
        });
    }

    template <typename FinishedCallback>
    void getJSON2(FinishedCallback onFinished)
    {
        this->get([onFinished{std::move(onFinished)}](const QByteArray &bytes) {
            auto object = parseJSONFromData2(bytes);
            onFinished(object);
        });
    }
};

}  // namespace util
}  // namespace chatterino
