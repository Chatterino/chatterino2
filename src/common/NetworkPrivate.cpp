#include "common/NetworkPrivate.hpp"

#include "common/NetworkManager.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/Paths.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"

#include <QCryptographicHash>
#include <QFile>
#include <QNetworkReply>
#include <QtConcurrent>
#include "common/QLogging.hpp"

namespace chatterino {

NetworkData::NetworkData()
    : lifetimeManager_(new QObject)
{
    DebugCount::increase("NetworkData");
}

NetworkData::~NetworkData()
{
    this->lifetimeManager_->deleteLater();

    DebugCount::decrease("NetworkData");
}

QString NetworkData::getHash()
{
    static std::mutex mu;

    std::lock_guard lock(mu);

    if (this->hash_.isEmpty())
    {
        QByteArray bytes;

        bytes.append(this->request_.url().toString().toUtf8());

        for (const auto &header : this->request_.rawHeaderList())
        {
            bytes.append(header);
        }

        QByteArray hashBytes(
            QCryptographicHash::hash(bytes, QCryptographicHash::Sha256));

        this->hash_ = hashBytes.toHex();
    }

    return this->hash_;
}

void writeToCache(const std::shared_ptr<NetworkData> &data,
                  const QByteArray &bytes)
{
    if (data->cache_)
    {
        QtConcurrent::run([data, bytes] {
            QFile cachedFile(getPaths()->cacheDirectory() + "/" +
                             data->getHash());

            if (cachedFile.open(QIODevice::WriteOnly))
            {
                cachedFile.write(bytes);
            }
        });
    }
}

void loadUncached(const std::shared_ptr<NetworkData> &data)
{
    DebugCount::increase("http request started");

    NetworkRequester requester;
    NetworkWorker *worker = new NetworkWorker;

    worker->moveToThread(&NetworkManager::workerThread);

    auto onUrlRequested = [data, worker]() mutable {
        if (data->hasTimeout_)
        {
            data->timer_ = new QTimer();
            data->timer_->setSingleShot(true);
            data->timer_->start(data->timeoutMS_);
        }

        auto reply = [&]() -> QNetworkReply * {
            switch (data->requestType_)
            {
                case NetworkRequestType::Get:
                    return NetworkManager::accessManager.get(data->request_);

                case NetworkRequestType::Put:
                    return NetworkManager::accessManager.put(data->request_,
                                                             data->payload_);

                case NetworkRequestType::Delete:
                    return NetworkManager::accessManager.deleteResource(
                        data->request_);

                case NetworkRequestType::Post:
                    if (data->multiPartPayload_)
                    {
                        assert(data->payload_.isNull());

                        return NetworkManager::accessManager.post(
                            data->request_, data->multiPartPayload_);
                    }
                    else
                    {
                        return NetworkManager::accessManager.post(
                            data->request_, data->payload_);
                    }
                case NetworkRequestType::Patch:
                    if (data->multiPartPayload_)
                    {
                        assert(data->payload_.isNull());

                        return NetworkManager::accessManager.sendCustomRequest(
                            data->request_, "PATCH", data->multiPartPayload_);
                    }
                    else
                    {
                        return NetworkManager::accessManager.sendCustomRequest(
                            data->request_, "PATCH", data->payload_);
                    }
            }
            return nullptr;
        }();

        if (reply == nullptr)
        {
            qCDebug(chatterinoCommon) << "Unhandled request type";
            return;
        }

        if (data->timer_ != nullptr && data->timer_->isActive())
        {
            QObject::connect(
                data->timer_, &QTimer::timeout, worker, [reply, data]() {
                    qCDebug(chatterinoCommon) << "Aborted!";
                    reply->abort();
                    qCDebug(chatterinoHTTP)
                        << QString("%1 [timed out] %2")
                               .arg(networkRequestTypes.at(
                                        int(data->requestType_)),
                                    data->request_.url().toString());

                    if (data->onError_)
                    {
                        postToThread([data] {
                            data->onError_(NetworkResult(
                                {}, NetworkResult::timedoutStatus));
                        });
                    }

                    if (data->finally_)
                    {
                        postToThread([data] {
                            data->finally_();
                        });
                    }
                });
        }

        if (data->onReplyCreated_)
        {
            data->onReplyCreated_(reply);
        }

        auto handleReply = [data, reply]() mutable {
            if (data->hasCaller_ && !data->caller_.get())
            {
                return;
            }

            // TODO(pajlada): A reply was received, kill the timeout timer
            if (reply->error() != QNetworkReply::NetworkError::NoError)
            {
                if (reply->error() ==
                    QNetworkReply::NetworkError::OperationCanceledError)
                {
                    // Operation cancelled, most likely timed out
                    qCDebug(chatterinoHTTP)
                        << QString("%1 [cancelled] %2")
                               .arg(networkRequestTypes.at(
                                        int(data->requestType_)),
                                    data->request_.url().toString());
                    return;
                }

                if (data->onError_)
                {
                    auto status = reply->attribute(
                        QNetworkRequest::HttpStatusCodeAttribute);
                    if (data->requestType_ == NetworkRequestType::Get)
                    {
                        qCDebug(chatterinoHTTP)
                            << QString("%1 %2 %3")
                                   .arg(networkRequestTypes.at(
                                            int(data->requestType_)),
                                        QString::number(status.toInt()),
                                        data->request_.url().toString());
                    }
                    else
                    {
                        qCDebug(chatterinoHTTP)
                            << QString("%1 %2 %3 %4")
                                   .arg(networkRequestTypes.at(
                                            int(data->requestType_)),
                                        QString::number(status.toInt()),
                                        data->request_.url().toString(),
                                        QString(data->payload_));
                    }
                    // TODO: Should this always be run on the GUI thread?
                    postToThread([data, code = status.toInt()] {
                        data->onError_(NetworkResult({}, code));
                    });
                }

                if (data->finally_)
                {
                    postToThread([data] {
                        data->finally_();
                    });
                }
                return;
            }

            QByteArray bytes = reply->readAll();
            writeToCache(data, bytes);

            auto status =
                reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

            NetworkResult result(bytes, status.toInt());

            DebugCount::increase("http request success");
            // log("starting {}", data->request_.url().toString());
            if (data->onSuccess_)
            {
                if (data->executeConcurrently_)
                    QtConcurrent::run([onSuccess = std::move(data->onSuccess_),
                                       result = std::move(result)] {
                        onSuccess(result);
                    });
                else
                    data->onSuccess_(result);
            }
            // log("finished {}", data->request_.url().toString());

            reply->deleteLater();

            if (data->requestType_ == NetworkRequestType::Get)
            {
                qCDebug(chatterinoHTTP)
                    << QString("%1 %2 %3")
                           .arg(networkRequestTypes.at(int(data->requestType_)),
                                QString::number(status.toInt()),
                                data->request_.url().toString());
            }
            else
            {
                qCDebug(chatterinoHTTP)
                    << QString("%1 %3 %2 %4")
                           .arg(networkRequestTypes.at(int(data->requestType_)),
                                data->request_.url().toString(),
                                QString::number(status.toInt()),
                                QString(data->payload_));
            }
            if (data->finally_)
            {
                if (data->executeConcurrently_)
                    QtConcurrent::run([finally = std::move(data->finally_)] {
                        finally();
                    });
                else
                    data->finally_();
            }
        };

        if (data->timer_ != nullptr)
        {
            QObject::connect(reply, &QNetworkReply::finished, data->timer_,
                             &QObject::deleteLater);
        }

        QObject::connect(
            reply, &QNetworkReply::finished, worker,
            [data, handleReply, worker]() mutable {
                if (data->executeConcurrently_ || isGuiThread())
                {
                    handleReply();
                    delete worker;
                }
                else
                {
                    postToThread(
                        [worker, cb = std::move(handleReply)]() mutable {
                            cb();
                            delete worker;
                        });
                }
            });
    };

    QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                     onUrlRequested);

    emit requester.requestUrl();
}

// First tried to load cached, then uncached.
void loadCached(const std::shared_ptr<NetworkData> &data)
{
    QFile cachedFile(getPaths()->cacheDirectory() + "/" + data->getHash());

    if (!cachedFile.exists() || !cachedFile.open(QIODevice::ReadOnly))
    {
        // File didn't exist OR File could not be opened
        loadUncached(data);
        return;
    }
    else
    {
        // XXX: check if bytes is empty?
        QByteArray bytes = cachedFile.readAll();
        NetworkResult result(bytes, 200);

        qCDebug(chatterinoHTTP)
            << QString("%1 [CACHED] 200 %2")
                   .arg(networkRequestTypes.at(int(data->requestType_)),
                        data->request_.url().toString());
        if (data->onSuccess_)
        {
            if (data->executeConcurrently_ || isGuiThread())
            {
                // XXX: If outcome is Failure, we should invalidate the cache file
                // somehow/somewhere
                /*auto outcome =*/
                if (data->hasCaller_ && !data->caller_.get())
                {
                    return;
                }
                data->onSuccess_(result);
            }
            else
            {
                postToThread([data, result]() {
                    if (data->hasCaller_ && !data->caller_.get())
                    {
                        return;
                    }

                    data->onSuccess_(result);
                });
            }
        }

        if (data->finally_)
        {
            if (data->executeConcurrently_ || isGuiThread())
            {
                if (data->hasCaller_ && !data->caller_.get())
                {
                    return;
                }

                data->finally_();
            }
            else
            {
                postToThread([data]() {
                    if (data->hasCaller_ && !data->caller_.get())
                    {
                        return;
                    }

                    data->finally_();
                });
            }
        }
    }
}

void load(const std::shared_ptr<NetworkData> &data)
{
    if (data->cache_)
    {
        QtConcurrent::run(loadCached, data);
    }
    else
    {
        loadUncached(data);
    }
}

}  // namespace chatterino
