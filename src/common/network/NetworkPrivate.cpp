#include "common/network/NetworkPrivate.hpp"

#include "Application.hpp"
#include "common/network/NetworkManager.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/network/NetworkTask.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "util/AbandonObject.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"
#include "util/QMagicEnum.hpp"

#include <magic_enum/magic_enum.hpp>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QNetworkReply>
#include <QtConcurrent>

#ifdef NDEBUG
constexpr qsizetype SLOW_HTTP_THRESHOLD = 30;
#else
constexpr qsizetype SLOW_HTTP_THRESHOLD = 90;
#endif

using namespace chatterino::network::detail;

namespace {

using namespace chatterino;

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

void loadUncached(std::shared_ptr<NetworkData> &&data)
{
    DebugCount::increase("http request started");

    NetworkRequester requester;
    auto *worker = new NetworkTask(std::move(data));

    worker->moveToThread(NetworkManager::workerThread);

    QObject::connect(&requester, &NetworkRequester::requestUrl, worker,
                     &NetworkTask::run);

    requester.requestUrl();
}

void loadCached(std::shared_ptr<NetworkData> &&data)
{
    QFile cachedFile(getApp()->getPaths().cacheDirectory() + "/" +
                     data->getHash());

    if (!cachedFile.exists() || !cachedFile.open(QIODevice::ReadOnly))
    {
        loadUncached(std::move(data));
        return;
    }

    // XXX: check if bytes is empty?
    QByteArray bytes = cachedFile.readAll();

    qCDebug(chatterinoHTTP).noquote() << data->typeString() << "[CACHED] 200"
                                      << data->request.url().toString();

    data->emitSuccess(
        {NetworkResult::NetworkError::NoError, QVariant(200), bytes});
    data->emitFinally();
}

}  // namespace

namespace chatterino {

NetworkData::NetworkData()
{
    DebugCount::increase("NetworkData");
}

NetworkData::~NetworkData()
{
    DebugCount::decrease("NetworkData");
}

QString NetworkData::getHash()
{
    if (this->hash_.isEmpty())
    {
        QByteArray bytes;

        bytes.append(this->request.url().toString().toUtf8());

        for (const auto &header : this->request.rawHeaderList())
        {
            bytes.append(header);
        }

        QByteArray hashBytes(
            QCryptographicHash::hash(bytes, QCryptographicHash::Sha256));

        this->hash_ = hashBytes.toHex();
    }

    return this->hash_;
}

void NetworkData::emitSuccess(NetworkResult &&result)
{
    if (!this->onSuccess)
    {
        return;
    }

    runCallback(this->executeConcurrently,
                [cb = std::move(this->onSuccess), result = std::move(result),
                 url = this->request.url(), hasCaller = this->hasCaller,
                 caller = this->caller]() {
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
}

void NetworkData::emitError(NetworkResult &&result)
{
    if (!this->onError)
    {
        return;
    }

    runCallback(this->executeConcurrently,
                [cb = std::move(this->onError), result = std::move(result),
                 hasCaller = this->hasCaller, caller = this->caller]() {
                    if (hasCaller && caller.isNull())
                    {
                        return;
                    }

                    cb(result);
                });
}

void NetworkData::emitFinally()
{
    if (!this->finally)
    {
        return;
    }

    runCallback(this->executeConcurrently,
                [cb = std::move(this->finally), hasCaller = this->hasCaller,
                 caller = this->caller]() {
                    if (hasCaller && caller.isNull())
                    {
                        return;
                    }

                    cb();
                });
}

QString NetworkData::typeString() const
{
    return qmagicenum::enumNameString(this->requestType);
}

void load(std::shared_ptr<NetworkData> &&data)
{
    if (data->cache)
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

}  // namespace chatterino
