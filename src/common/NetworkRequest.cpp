#include "common/NetworkRequest.hpp"

#include "Application.hpp"

namespace chatterino {

NetworkRequest::NetworkRequest(const char *url)
{
    this->data.request.setUrl(QUrl(url));
}

NetworkRequest::NetworkRequest(const std::string &url)
{
    this->data.request.setUrl(QUrl(QString::fromStdString(url)));
}

NetworkRequest::NetworkRequest(const QString &url)
{
    this->data.request.setUrl(QUrl(url));
}

void NetworkRequest::setRequestType(RequestType newRequestType)
{
    this->data.requestType = newRequestType;
}

void NetworkRequest::setCaller(const QObject *_caller)
{
    this->data.caller = _caller;
}

void NetworkRequest::setOnReplyCreated(std::function<void(QNetworkReply *)> f)
{
    this->data.onReplyCreated = f;
}

void NetworkRequest::setRawHeader(const char *headerName, const char *value)
{
    this->data.request.setRawHeader(headerName, value);
}

void NetworkRequest::setRawHeader(const char *headerName, const QByteArray &value)
{
    this->data.request.setRawHeader(headerName, value);
}

void NetworkRequest::setRawHeader(const char *headerName, const QString &value)
{
    this->data.request.setRawHeader(headerName, value.toUtf8());
}

void NetworkRequest::setTimeout(int ms)
{
    this->data.timeoutMS = ms;
}

void NetworkRequest::makeAuthorizedV5(const QString &clientID, const QString &oauthToken)
{
    this->setRawHeader("Client-ID", clientID);
    this->setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    this->setRawHeader("Authorization", "OAuth " + oauthToken);
}

void NetworkRequest::setUseQuickLoadCache(bool value)
{
    this->data.useQuickLoadCache = value;
}

void NetworkRequest::Data::writeToCache(const QByteArray &bytes)
{
    if (this->useQuickLoadCache) {
        auto app = getApp();

        QFile cachedFile(app->paths->cacheDirectory + "/" + this->getHash());

        if (cachedFile.open(QIODevice::WriteOnly)) {
            cachedFile.write(bytes);

            cachedFile.close();
        }
    }
}

void NetworkRequest::execute()
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

void NetworkRequest::useCache()
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

void NetworkRequest::doRequest()
{
    QTimer *timer = nullptr;
    if (this->data.timeoutMS > 0) {
        timer = new QTimer;
    }

    NetworkRequester requester;
    NetworkWorker *worker = new NetworkWorker;

    worker->moveToThread(&NetworkManager::workerThread);

    if (this->data.caller != nullptr) {
        QObject::connect(worker, &NetworkWorker::doneUrl,
                         this->data.caller, [data = this->data](auto reply) mutable {
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
                     [ timer, data = std::move(this->data), worker ]() {
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
                                          [ data = std::move(data), worker, reply ]() mutable {
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

void NetworkRequest::executeGet()
{
    this->useCache();

    this->doRequest();
}

void NetworkRequest::executePut()
{
    this->doRequest();
}

void NetworkRequest::executeDelete()
{
    this->doRequest();
}
}  // namespace chatterino
