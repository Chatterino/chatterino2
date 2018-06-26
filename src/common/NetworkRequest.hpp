#pragma once

#include "Application.hpp"
#include "singletons/PathManager.hpp"
#include "common/NetworkManager.hpp"
#include "common/NetworkRequester.hpp"
#include "common/NetworkWorker.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <QCryptographicHash>
#include <QFile>

namespace chatterino {

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
        Log("JSON parse error: {} ({})", rapidjson::GetParseError_En(result.Code()),
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
        Log("JSON parse error: {} ({})", rapidjson::GetParseError_En(result.Code()),
                   result.Offset());
        return ret;
    }

    return ret;
}

class NetworkRequest
{
public:
    enum RequestType {
        GetRequest,
        PostRequest,
        PutRequest,
        DeleteRequest,
    };

private:
    struct Data {
        QNetworkRequest request;
        const QObject *caller = nullptr;
        std::function<void(QNetworkReply *)> onReplyCreated;
        int timeoutMS = -1;
        bool useQuickLoadCache = false;

        std::function<bool(int)> onError;
        std::function<bool(const rapidjson::Document &)> onSuccess;

        NetworkRequest::RequestType requestType;

        QByteArray payload;

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

    void setRequestType(RequestType newRequestType)
    {
        this->data.requestType = newRequestType;
    }

    template <typename Func>
    void onError(Func cb)
    {
        this->data.onError = cb;
    }

    template <typename Func>
    void onSuccess(Func cb)
    {
        this->data.onSuccess = cb;
    }

    void setPayload(const QByteArray &payload)
    {
        this->data.payload = payload;
    }

    void setUseQuickLoadCache(bool value);

    void setCaller(const QObject *_caller)
    {
        this->data.caller = _caller;
    }

    void setOnReplyCreated(std::function<void(QNetworkReply *)> f)
    {
        this->data.onReplyCreated = f;
    }

    void setRawHeader(const char *headerName, const char *value)
    {
        this->data.request.setRawHeader(headerName, value);
    }

    void setRawHeader(const char *headerName, const QByteArray &value)
    {
        this->data.request.setRawHeader(headerName, value);
    }

    void setRawHeader(const char *headerName, const QString &value)
    {
        this->data.request.setRawHeader(headerName, value.toUtf8());
    }

    void setTimeout(int ms)
    {
        this->data.timeoutMS = ms;
    }

    void makeAuthorizedV5(const QString &clientID, const QString &oauthToken)
    {
        this->setRawHeader("Client-ID", clientID);
        this->setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
        this->setRawHeader("Authorization", "OAuth " + oauthToken);
    }

    template <typename FinishedCallback>
    void get(FinishedCallback onFinished)
    {
        if (this->data.useQuickLoadCache) {
            auto app = getApp();

            QFile cachedFile(app->paths->cacheDirectory + "/" + this->data.getHash());

            if (cachedFile.exists()) {
                if (cachedFile.open(QIODevice::ReadOnly)) {
                    QByteArray bytes = cachedFile.readAll();

                    // qDebug() << "Loaded cached resource" << this->data.request.url();

                    bool success = onFinished(bytes);

                    cachedFile.close();

                    if (!success) {
                        // The images were not successfully loaded from the file
                        // XXX: Invalidate the cache file so we don't attempt to load it again next
                        // time
                    }
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

                                 QByteArray readBytes = reply->readAll();
                                 QByteArray bytes;
                                 bytes.setRawData(readBytes.data(), readBytes.size());
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
                        Log("Aborted!");
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
        this->get([onFinished{std::move(onFinished)}](const QByteArray &bytes) -> bool {
            auto object = parseJSONFromData(bytes);
            onFinished(object);

            // XXX: Maybe return onFinished? For now I don't want to force onFinished to have a
            // return value
            return true;
        });
    }

    template <typename FinishedCallback>
    void getJSON2(FinishedCallback onFinished)
    {
        this->get([onFinished{std::move(onFinished)}](const QByteArray &bytes) -> bool {
            auto object = parseJSONFromData2(bytes);
            onFinished(object);

            // XXX: Maybe return onFinished? For now I don't want to force onFinished to have a
            // return value
            return true;
        });
    }

    void execute()
    {
        switch (this->data.requestType) {
            case GetRequest: {
                this->executeGet();
            } break;

            case PutRequest: {
                this->executePut();
            } break;

            case DeleteRequest: {
                this->executeDelete();
            } break;

            default: {
                Log("[Execute] Unhandled request type {}", (int)this->data.requestType);
            } break;
        }
    }

private:
    void useCache()
    {
        if (this->data.useQuickLoadCache) {
            auto app = getApp();

            QFile cachedFile(app->paths->cacheDirectory + "/" + this->data.getHash());

            if (cachedFile.exists()) {
                if (cachedFile.open(QIODevice::ReadOnly)) {
                    QByteArray bytes = cachedFile.readAll();

                    // qDebug() << "Loaded cached resource" << this->data.request.url();

                    auto document = parseJSONFromData2(bytes);

                    bool success = false;

                    if (!document.IsNull()) {
                        success = this->data.onSuccess(document);
                    }

                    cachedFile.close();

                    if (!success) {
                        // The images were not successfully loaded from the file
                        // XXX: Invalidate the cache file so we don't attempt to load it again next
                        // time
                    }
                }
            }
        }
    }

    void doRequest()
    {
        QTimer *timer = nullptr;
        if (this->data.timeoutMS > 0) {
            timer = new QTimer;
        }

        NetworkRequester requester;
        NetworkWorker *worker = new NetworkWorker;

        worker->moveToThread(&NetworkManager::workerThread);

        if (this->data.caller != nullptr) {
            QObject::connect(worker, &NetworkWorker::doneUrl, this->data.caller,
                             [data = this->data](auto reply) mutable {
                                 if (reply->error() != QNetworkReply::NetworkError::NoError) {
                                     if (data.onError) {
                                         data.onError(reply->error());
                                     }
                                     return;
                                 }

                                 QByteArray readBytes = reply->readAll();
                                 QByteArray bytes;
                                 bytes.setRawData(readBytes.data(), readBytes.size());
                                 data.writeToCache(bytes);
                                 data.onSuccess(parseJSONFromData2(bytes));

                                 reply->deleteLater();
                             });
        }

        if (timer != nullptr) {
            timer->start(this->data.timeoutMS);
        }

        QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                         [timer, data = std::move(this->data), worker]() {
                             QNetworkReply *reply = nullptr;
                             switch (data.requestType) {
                                 case GetRequest: {
                                     reply = NetworkManager::NaM.get(data.request);
                                 } break;

                                 case PutRequest: {
                                     reply = NetworkManager::NaM.put(data.request, data.payload);
                                 } break;

                                 case DeleteRequest: {
                                     reply = NetworkManager::NaM.deleteResource(data.request);
                                 } break;
                             }

                             if (reply == nullptr) {
                                 Log("Unhandled request type {}", (int)data.requestType);
                                 return;
                             }

                             if (timer != nullptr) {
                                 QObject::connect(timer, &QTimer::timeout, worker,
                                                  [reply, timer, data]() {
                                                      Log("Aborted!");
                                                      reply->abort();
                                                      timer->deleteLater();
                                                      data.onError(-2);
                                                  });
                             }

                             if (data.onReplyCreated) {
                                 data.onReplyCreated(reply);
                             }

                             QObject::connect(reply, &QNetworkReply::finished, worker,
                                              [data = std::move(data), worker, reply]() mutable {
                                                  if (data.caller == nullptr) {
                                                      QByteArray bytes = reply->readAll();
                                                      data.writeToCache(bytes);

                                                      if (data.onSuccess) {
                                                          data.onSuccess(parseJSONFromData2(bytes));
                                                      } else {
                                                          qWarning() << "data.onSuccess not found";
                                                      }

                                                      reply->deleteLater();
                                                  } else {
                                                      emit worker->doneUrl(reply);
                                                  }

                                                  delete worker;
                                              });
                         });

        emit requester.requestUrl();
    }

    void executeGet()
    {
        this->useCache();

        this->doRequest();
    }

    void executePut()
    {
        this->doRequest();
    }

    void executeDelete()
    {
        this->doRequest();
    }
};

}  // namespace chatterino
