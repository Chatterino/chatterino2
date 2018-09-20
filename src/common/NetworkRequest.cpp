#include "common/NetworkRequest.hpp"

#include "common/NetworkData.hpp"
#include "common/NetworkManager.hpp"
#include "common/Outcome.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Paths.hpp"
#include "util/DebugCount.hpp"

#include <QFile>
#include <QtConcurrent>

#include <cassert>

namespace chatterino {

NetworkRequest::NetworkRequest(const std::string &url,
                               NetworkRequestType requestType)
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

void NetworkRequest::setRawHeader(const char *headerName,
                                  const QByteArray &value)
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

void NetworkRequest::setExecuteConcurrently(bool value)
{
    this->data->executeConcurrently = value;
}

void NetworkRequest::makeAuthorizedV5(const QString &clientID,
                                      const QString &oauthToken)
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
            // Put requests cannot be cached, therefore the request is called
            // immediately
            this->doRequest();
        } break;

        case NetworkRequestType::Delete: {
            // Delete requests cannot be cached, therefore the request is called
            // immediately
            this->doRequest();
        } break;

        default: {
            log("[Execute] Unhandled request type");
        } break;
    }
}

QString NetworkRequest::urlString() const
{
    return this->data->request_.url().toString();
}

Outcome NetworkRequest::tryLoadCachedFile()
{
    QFile cachedFile(getPaths()->cacheDirectory() + "/" +
                     this->data->getHash());

    if (!cachedFile.exists()) {
        // File didn't exist
        return Failure;
    }

    if (!cachedFile.open(QIODevice::ReadOnly)) {
        // File could not be opened
        return Failure;
    }

    QByteArray bytes = cachedFile.readAll();
    NetworkResult result(bytes);

    auto outcome = this->data->onSuccess_(result);

    cachedFile.close();

    // XXX: If success is false, we should invalidate the cache file
    // somehow/somewhere

    return outcome;
}

void NetworkRequest::doRequest()
{
    DebugCount::increase("http request started");

    NetworkRequester requester;
    NetworkWorker *worker = new NetworkWorker;

    worker->moveToThread(&NetworkManager::workerThread);

    this->timer->start();

    auto onUrlRequested = [data = this->data, timer = this->timer,
                           worker]() mutable {
        auto reply = [&]() -> QNetworkReply * {
            switch (data->requestType_) {
                case NetworkRequestType::Get:
                    return NetworkManager::accessManager.get(data->request_);

                case NetworkRequestType::Put:
                    return NetworkManager::accessManager.put(data->request_,
                                                             data->payload_);

                case NetworkRequestType::Delete:
                    return NetworkManager::accessManager.deleteResource(
                        data->request_);

                default:
                    return nullptr;
            }
        }();

        if (reply == nullptr) {
            log("Unhandled request type");
            return;
        }

        if (timer->isStarted()) {
            timer->onTimeout(worker, [reply, data]() {
                log("Aborted!");
                reply->abort();
                if (data->onError_) {
                    data->onError_(-2);
                }
            });
        }

        if (data->onReplyCreated_) {
            data->onReplyCreated_(reply);
        }

        auto handleReply = [data, timer, reply]() mutable {
            // TODO(pajlada): A reply was received, kill the timeout timer
            if (reply->error() != QNetworkReply::NetworkError::NoError) {
                if (data->onError_) {
                    data->onError_(reply->error());
                }
                return;
            }

            QByteArray bytes = reply->readAll();
            data->writeToCache(bytes);

            NetworkResult result(bytes);

            DebugCount::increase("http request success");
            // log("starting {}", data->request_.url().toString());
            if (data->onSuccess_) {
                if (data->executeConcurrently)
                    QtConcurrent::run(
                        [onSuccess = std::move(data->onSuccess_),
                         result = std::move(result)] { onSuccess(result); });
                else
                    data->onSuccess_(result);
            }
            // log("finished {}", data->request_.url().toString());

            reply->deleteLater();
        };

        if (data->caller_ != nullptr) {
            QObject::connect(worker, &NetworkWorker::doneUrl, data->caller_,
                             handleReply);
            QObject::connect(reply, &QNetworkReply::finished, worker,
                             [worker]() mutable {
                                 emit worker->doneUrl();

                                 delete worker;
                             });
        } else {
            QObject::connect(reply, &QNetworkReply::finished, worker,
                             [handleReply, worker]() mutable {
                                 handleReply();

                                 delete worker;
                             });
        }
    };

    QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                     onUrlRequested);

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
