#include "common/NetworkPrivate.hpp"

#include "common/NetworkManager.hpp"
#include "common/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"

#include <magic_enum/magic_enum.hpp>
#include <QCryptographicHash>
#include <QFile>
#include <QNetworkReply>
#include <QtConcurrent>

#ifndef NDEBUG
#    define CHATTERINO_WARN_SLOW_HTTP
#endif

#ifdef CHATTERINO_WARN_SLOW_HTTP
#    include <QElapsedTimer>

#    ifdef NDEBUG
constexpr qsizetype SLOW_HTTP_THRESHOLD = 30;
#    else
constexpr qsizetype SLOW_HTTP_THRESHOLD = 90;
#    endif
#endif

namespace {

using namespace chatterino;

/// Guard to call `deleteLater` on a QObject when destroyed.
class AbandonObject
{
public:
    AbandonObject(QObject *obj)
        : obj_(obj)
    {
    }

    ~AbandonObject()
    {
        if (this->obj_)
        {
            this->obj_->deleteLater();
        }
    }

    AbandonObject(const AbandonObject &) = delete;
    AbandonObject(AbandonObject &&) = delete;
    AbandonObject &operator=(const AbandonObject &) = delete;
    AbandonObject &operator=(AbandonObject &&) = delete;

private:
    QObject *obj_;
};

class NetworkTask : public QObject
{
    Q_OBJECT

public:
    NetworkTask(std::shared_ptr<NetworkData> &&data)
        : data_(std::move(data))
    {
    }
    ~NetworkTask() override
    {
        if (this->reply_)
        {
            this->reply_->deleteLater();
        }
    }
    NetworkTask(const NetworkTask &) = delete;
    NetworkTask(NetworkTask &&) = delete;
    NetworkTask &operator=(const NetworkTask &) = delete;
    NetworkTask &operator=(NetworkTask &&) = delete;

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
    void run();

private:
    QNetworkReply *createReply();

    void logReply();
    void writeToCache(const QByteArray &bytes) const;

    std::shared_ptr<NetworkData> data_;
    QNetworkReply *reply_{};  // parent: default (accessManager)
    QTimer *timer_{};         // parent: this

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
private slots:
    void timeout();
    void finished();
};

void runCallback(bool concurrent, auto &&fn)
{
    if (concurrent)
    {
        std::ignore = QtConcurrent::run(std::forward<decltype(fn)>(fn));
    }
    else
    {
        runInGuiThread(std::forward<decltype(fn)>(fn));
    }
}

}  // namespace

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

void loadUncached(std::shared_ptr<NetworkData> &&data)
{
    DebugCount::increase("http request started");

    NetworkRequester requester;
    auto *worker = new NetworkTask(std::move(data));

    worker->moveToThread(&NetworkManager::workerThread);

    QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                     &NetworkTask::run);

    emit requester.requestUrl();
}

// First tried to load cached, then uncached.
void loadCached(std::shared_ptr<NetworkData> &&data)
{
    QFile cachedFile(getPaths()->cacheDirectory() + "/" + data->getHash());

    if (!cachedFile.exists() || !cachedFile.open(QIODevice::ReadOnly))
    {
        // File didn't exist OR File could not be opened
        loadUncached(std::move(data));
        return;
    }

    // XXX: check if bytes is empty?
    QByteArray bytes = cachedFile.readAll();

    qCDebug(chatterinoHTTP).noquote() << data->typeString() << "[CACHED] 200"
                                      << data->request_.url().toString();

    data->emitSuccess(
        {NetworkResult::NetworkError::NoError, QVariant(200), bytes});
    data->emitFinally();
}

void load(std::shared_ptr<NetworkData> &&data)
{
    if (data->cache_)
    {
        std::ignore = QtConcurrent::run([data = std::move(data)]() mutable {
            loadCached(std::move(data));
        });
    }
    else
    {
        loadUncached(std::move(data));
    }
}

void NetworkData::emitSuccess(NetworkResult &&result)
{
    if (!this->onSuccess_)
    {
        return;
    }

#ifdef CHATTERINO_WARN_SLOW_HTTP
    if (!this->executeConcurrently_)
    {
        runCallback(
            false, [cb = std::move(this->onSuccess_),
                    result = std::move(result), url = this->request_.url(),
                    hasCaller = this->hasCaller_, caller = this->caller_]() {
                if (hasCaller && caller.isNull())
                {
                    return;
                }

                QElapsedTimer timer;
                timer.start();
                cb(result);
                if (timer.elapsed() > SLOW_HTTP_THRESHOLD)
                {
                    qCWarning(chatterinoHTTP)
                        << "Slow HTTP success handler for" << url.toString()
                        << timer.elapsed()
                        << "ms (threshold:" << SLOW_HTTP_THRESHOLD << "ms)";
                }
            });
        return;
    }
#endif

    runCallback(this->executeConcurrently_,
                [cb = std::move(this->onSuccess_), result = std::move(result),
                 hasCaller = this->hasCaller_, caller = this->caller_]() {
                    if (hasCaller && caller.isNull())
                    {
                        return;
                    }

                    cb(result);
                });
}

void NetworkData::emitError(NetworkResult &&result)
{
    if (!this->onError_)
    {
        return;
    }

    runCallback(this->executeConcurrently_,
                [cb = std::move(this->onError_), result = std::move(result),
                 hasCaller = this->hasCaller_, caller = this->caller_]() {
                    if (hasCaller && caller.isNull())
                    {
                        return;
                    }

                    cb(result);
                });
}

void NetworkData::emitFinally()
{
    if (!this->finally_)
    {
        return;
    }

    runCallback(this->executeConcurrently_,
                [cb = std::move(this->finally_), hasCaller = this->hasCaller_,
                 caller = this->caller_]() {
                    if (hasCaller && caller.isNull())
                    {
                        return;
                    }

                    cb();
                });
}

QLatin1String NetworkData::typeString() const
{
    auto view = magic_enum::enum_name<NetworkRequestType>(this->requestType_);
    return QLatin1String{view.data(),
                         static_cast<QLatin1String::size_type>(view.size())};
}

}  // namespace chatterino

namespace {

void NetworkTask::run()
{
    if (this->data_->hasTimeout_)
    {
        this->timer_ = new QTimer(this);
        this->timer_->setSingleShot(true);
        this->timer_->start(this->data_->timeoutMS_);
        QObject::connect(this->timer_, &QTimer::timeout, this,
                         &NetworkTask::timeout);
    }

    this->reply_ = this->createReply();
    if (!this->reply_)
    {
        this->deleteLater();
        return;
    }
    QObject::connect(this->reply_, &QNetworkReply::finished, this,
                     &NetworkTask::finished);
}

QNetworkReply *NetworkTask::createReply()
{
    const auto &data = this->data_;
    const auto &request = this->data_->request_;
    auto &accessManager = NetworkManager::accessManager;
    switch (this->data_->requestType_)
    {
        case NetworkRequestType::Get:
            return accessManager.get(request);

        case NetworkRequestType::Put:
            return accessManager.put(request, data->payload_);

        case NetworkRequestType::Delete:
            return accessManager.deleteResource(data->request_);

        case NetworkRequestType::Post:
            if (data->multiPartPayload_)
            {
                assert(data->payload_.isNull());

                return accessManager.post(request, data->multiPartPayload_);
            }
            else
            {
                return accessManager.post(request, data->payload_);
            }
        case NetworkRequestType::Patch:
            if (data->multiPartPayload_)
            {
                assert(data->payload_.isNull());

                return accessManager.sendCustomRequest(request, "PATCH",
                                                       data->multiPartPayload_);
            }
            else
            {
                return NetworkManager::accessManager.sendCustomRequest(
                    request, "PATCH", data->payload_);
            }
    }
    return nullptr;
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
        << this->data_->request_.url().toString();

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

    if (reply->error() != QNetworkReply::NetworkError::NoError)
    {
        if (reply->error() ==
            QNetworkReply::NetworkError::OperationCanceledError)
        {
            // Operation cancelled, most likely timed out
            qCDebug(chatterinoHTTP).noquote()
                << this->data_->typeString() << "[cancelled]"
                << this->data_->request_.url().toString();
            return;
        }

        this->logReply();
        this->data_->emitError({reply->error(), status, reply->readAll()});
        this->data_->emitFinally();

        return;
    }

    QByteArray bytes = reply->readAll();

    if (this->data_->cache_)
    {
        this->writeToCache(bytes);
    }

    DebugCount::increase("http request success");
    this->logReply();
    this->data_->emitSuccess({reply->error(), status, bytes});
    this->data_->emitFinally();
}

void NetworkTask::logReply()
{
    auto status =
        this->reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute)
            .toInt();
    if (this->data_->requestType_ == NetworkRequestType::Get)
    {
        qCDebug(chatterinoHTTP).noquote()
            << this->data_->typeString() << status
            << this->data_->request_.url().toString();
    }
    else
    {
        qCDebug(chatterinoHTTP).noquote()
            << this->data_->typeString()
            << this->data_->request_.url().toString() << status
            << QString(this->data_->payload_);
    }
}

void NetworkTask::writeToCache(const QByteArray &bytes) const
{
    std::ignore = QtConcurrent::run([data = this->data_, bytes] {
        QFile cachedFile(getPaths()->cacheDirectory() + "/" + data->getHash());

        if (cachedFile.open(QIODevice::WriteOnly))
        {
            cachedFile.write(bytes);
        }
    });
}

}  // namespace

#include "NetworkPrivate.moc"
