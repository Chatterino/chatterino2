#include "common/NetworkRequest.hpp"

#include "Application.hpp"
#include "common/NetworkManager.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Paths.hpp"

#include <QFile>

#include <cassert>

namespace chatterino {

NetworkRequest::NetworkRequest(const std::string &url, NetworkRequestType requestType)
    : data(new NetworkData)
    , timer(new NetworkTimer)
{
    this->data->request_.setUrl(QUrl(QString::fromStdString(url)));
    this->data->requestType_ = requestType;
}

NetworkRequest::NetworkRequest(QUrl url, NetworkRequestType requestType)
    : data(new NetworkData)
    , timer(new NetworkTimer)
{
    this->data->request_.setUrl(url);
    this->data->requestType_ = requestType;
}

NetworkRequest::~NetworkRequest()
{
    //    assert(this->executed_);
}

void NetworkRequest::setRequestType(NetworkRequestType newRequestType)
{
    this->data->requestType_ = newRequestType;
}

void NetworkRequest::setCaller(const QObject *caller)
{
    this->data->caller_ = caller;
}

void NetworkRequest::onReplyCreated(NetworkReplyCreatedCallback cb)
{
    this->data->onReplyCreated_ = cb;
}

void NetworkRequest::onError(NetworkErrorCallback cb)
{
    this->data->onError_ = cb;
}

void NetworkRequest::onSuccess(NetworkSuccessCallback cb)
{
    this->data->onSuccess_ = cb;
}

void NetworkRequest::setRawHeader(const char *headerName, const char *value)
{
    this->data->request_.setRawHeader(headerName, value);
}

void NetworkRequest::setRawHeader(const char *headerName, const QByteArray &value)
{
    this->data->request_.setRawHeader(headerName, value);
}

void NetworkRequest::setRawHeader(const char *headerName, const QString &value)
{
    this->data->request_.setRawHeader(headerName, value.toUtf8());
}

void NetworkRequest::setTimeout(int ms)
{
    this->timer->timeoutMS_ = ms;
}

void NetworkRequest::makeAuthorizedV5(const QString &clientID, const QString &oauthToken)
{
    this->setRawHeader("Client-ID", clientID);
    this->setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    if (!oauthToken.isEmpty()) {
        this->setRawHeader("Authorization", "OAuth " + oauthToken);
    }
}

void NetworkRequest::setPayload(const QByteArray &payload)
{
    this->data->payload_ = payload;
}

void NetworkRequest::setUseQuickLoadCache(bool value)
{
    this->data->useQuickLoadCache_ = value;
}

void NetworkRequest::execute()
{
    this->executed_ = true;

    switch (this->data->requestType_) {
        case NetworkRequestType::Get: {
            // Get requests try to load from cache, then perform the request
            if (this->data->useQuickLoadCache_) {
                if (this->tryLoadCachedFile()) {
                    // Successfully loaded from cache
                    return;
                }
            }

            this->doRequest();
        } break;

        case NetworkRequestType::Put: {
            // Put requests cannot be cached, therefore the request is called immediately
            this->doRequest();
        } break;

        case NetworkRequestType::Delete: {
            // Delete requests cannot be cached, therefore the request is called immediately
            this->doRequest();
        } break;

        default: {
            Log("[Execute] Unhandled request type");
        } break;
    }
}

bool NetworkRequest::tryLoadCachedFile()
{
    auto app = getApp();

    QFile cachedFile(app->paths->cacheDirectory + "/" + this->data->getHash());

    if (!cachedFile.exists()) {
        // File didn't exist
        return false;
    }

    if (!cachedFile.open(QIODevice::ReadOnly)) {
        // File could not be opened
        return false;
    }

    QByteArray bytes = cachedFile.readAll();
    NetworkResult result(bytes);

    bool success = this->data->onSuccess_(result);

    cachedFile.close();

    // XXX: If success is false, we should invalidate the cache file somehow/somewhere

    return success;
}

void NetworkRequest::doRequest()
{
    NetworkRequester requester;
    NetworkWorker *worker = new NetworkWorker;

    worker->moveToThread(&NetworkManager::workerThread);

    this->timer->start();

    auto onUrlRequested = [data = this->data, timer = this->timer, worker]() mutable {
        QNetworkReply *reply = nullptr;
        switch (data->requestType_) {
            case NetworkRequestType::Get: {
                reply = NetworkManager::NaM.get(data->request_);
            } break;

            case NetworkRequestType::Put: {
                reply = NetworkManager::NaM.put(data->request_, data->payload_);
            } break;

            case NetworkRequestType::Delete: {
                reply = NetworkManager::NaM.deleteResource(data->request_);
            } break;
        }

        if (reply == nullptr) {
            Log("Unhandled request type");
            return;
        }

        if (timer->isStarted()) {
            timer->onTimeout(worker, [reply, data]() {
                Log("Aborted!");
                reply->abort();
                if (data->onError_) {
                    data->onError_(-2);
                }
            });
        }

        if (data->onReplyCreated_) {
            data->onReplyCreated_(reply);
        }

        bool directAction = (data->caller_ == nullptr);

        auto handleReply = [data, timer, reply]() mutable {
            // TODO(pajlada): A reply was received, kill the timeout timer
            if (reply->error() != QNetworkReply::NetworkError::NoError) {
                if (data->onError_) {
                    data->onError_(reply->error());
                }
                return;
            }

            QByteArray readBytes = reply->readAll();
            QByteArray bytes;
            bytes.setRawData(readBytes.data(), readBytes.size());
            data->writeToCache(bytes);

            NetworkResult result(bytes);
            data->onSuccess_(result);

            reply->deleteLater();
        };

        if (data->caller_ != nullptr) {
            QObject::connect(worker, &NetworkWorker::doneUrl, data->caller_,
                             std::move(handleReply));
            QObject::connect(reply, &QNetworkReply::finished, worker, [worker]() mutable {
                emit worker->doneUrl();

                delete worker;
            });
        } else {
            QObject::connect(reply, &QNetworkReply::finished, worker,
                             [handleReply = std::move(handleReply), worker]() mutable {
                                 handleReply();

                                 delete worker;
                             });
        }
    };

    QObject::connect(&requester, &NetworkRequester::requestUrl, worker, onUrlRequested);

    emit requester.requestUrl();
}

// Helper creator functions
NetworkRequest NetworkRequest::twitchRequest(QUrl url)
{
    NetworkRequest request(url);

    request.makeAuthorizedV5(getDefaultClientID());

    return request;
}

}  // namespace chatterino
