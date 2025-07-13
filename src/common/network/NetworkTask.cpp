#include "common/network/NetworkTask.hpp"

#include "Application.hpp"
#include "common/network/NetworkManager.hpp"
#include "common/network/NetworkPrivate.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "util/AbandonObject.hpp"
#include "util/DebugCount.hpp"

#include <QFile>
#include <QNetworkReply>
#include <QtConcurrent>

namespace chatterino::network::detail {

NetworkTask::NetworkTask(std::shared_ptr<NetworkData> &&data)
    : data_(std::move(data))
{
}

NetworkTask::~NetworkTask()
{
    if (this->reply_)
    {
        this->reply_->deleteLater();
    }
}

void NetworkTask::run()
{
    this->reply_ = this->createReply();
    if (!this->reply_)
    {
        this->deleteLater();
        return;
    }

    const auto &timeout = this->data_->timeout;
    if (timeout.has_value())
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QObject::connect(this->reply_, &QNetworkReply::requestSent, this,
                         [this]() {
                             const auto &timeout = this->data_->timeout;
                             this->timer_ = new QTimer(this);
                             this->timer_->setSingleShot(true);
                             this->timer_->start(timeout.value());
                             QObject::connect(this->timer_, &QTimer::timeout,
                                              this, &NetworkTask::timeout);
                         });
#else
        this->timer_ = new QTimer(this);
        this->timer_->setSingleShot(true);
        this->timer_->start(timeout.value());
        QObject::connect(this->timer_, &QTimer::timeout, this,
                         &NetworkTask::timeout);
#endif
    }

    QObject::connect(this->reply_, &QNetworkReply::finished, this,
                     &NetworkTask::finished);

#ifndef NDEBUG
    if (this->data_->ignoreSslErrors)
    {
        QObject::connect(this->reply_, &QNetworkReply::sslErrors, this,
                         [this](const auto &errors) {
                             this->reply_->ignoreSslErrors(errors);
                         });
    }
#endif
}

QNetworkReply *NetworkTask::createReply()
{
    const auto &data = this->data_;
    const auto &request = this->data_->request;
    auto *accessManager = NetworkManager::accessManager;
    switch (this->data_->requestType)
    {
        case NetworkRequestType::Get:
            return accessManager->get(request);

        case NetworkRequestType::Put:
            return accessManager->put(request, data->payload);

        case NetworkRequestType::Delete:
            return accessManager->deleteResource(data->request);

        case NetworkRequestType::Post:
            if (data->multiPartPayload)
            {
                assert(data->payload.isNull());

                return accessManager->post(request,
                                           data->multiPartPayload.get());
            }
            else
            {
                return accessManager->post(request, data->payload);
            }
        case NetworkRequestType::Patch:
            if (data->multiPartPayload)
            {
                assert(data->payload.isNull());

                return accessManager->sendCustomRequest(
                    request, "PATCH", data->multiPartPayload.get());
            }
            else
            {
                return NetworkManager::accessManager->sendCustomRequest(
                    request, "PATCH", data->payload);
            }
    }
    return nullptr;
}

void NetworkTask::logReply()
{
    auto status =
        this->reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute)
            .toInt();
    if (this->data_->requestType == NetworkRequestType::Get)
    {
        qCDebug(chatterinoHTTP).noquote()
            << this->data_->typeString() << status
            << this->data_->request.url().toString();
    }
    else
    {
        qCDebug(chatterinoHTTP).noquote()
            << this->data_->typeString()
            << this->data_->request.url().toString() << status
            << QString(this->data_->payload);
    }
}

void NetworkTask::writeToCache(const QByteArray &bytes) const
{
    std::ignore = QtConcurrent::run([data = this->data_, bytes] {
        QFile cachedFile(getApp()->getPaths().cacheDirectory() + "/" +
                         data->getHash());

        if (cachedFile.open(QIODevice::WriteOnly))
        {
            cachedFile.write(bytes);
        }
    });
}

void NetworkTask::timeout()
{
    AbandonObject guard(this);

    // prevent abort() from calling finished()
    QObject::disconnect(this->reply_, &QNetworkReply::finished, this,
                        &NetworkTask::finished);
    this->reply_->abort();

    qCDebug(chatterinoHTTP).noquote()
        << this->data_->typeString() << "[timed out]"
        << this->data_->request.url().toString();

    this->data_->emitError({NetworkResult::NetworkError::TimeoutError, {}, {}});
    this->data_->emitFinally();
}

void NetworkTask::finished()
{
    AbandonObject guard(this);

    if (this->timer_)
    {
        this->timer_->stop();
    }

    auto *reply = this->reply_;
    auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    if (reply->error() == QNetworkReply::OperationCanceledError)
    {
        // Operation cancelled, most likely timed out
        qCDebug(chatterinoHTTP).noquote()
            << this->data_->typeString() << "[cancelled]"
            << this->data_->request.url().toString();
        return;
    }

    if (reply->error() != QNetworkReply::NoError)
    {
        this->logReply();
        this->data_->emitError({reply->error(), status, reply->readAll()});
        this->data_->emitFinally();

        return;
    }

    QByteArray bytes = reply->readAll();

    if (this->data_->cache)
    {
        this->writeToCache(bytes);
    }

    DebugCount::increase("http request success");
    this->logReply();
    this->data_->emitSuccess({reply->error(), status, bytes});
    this->data_->emitFinally();
}

}  // namespace chatterino::network::detail
