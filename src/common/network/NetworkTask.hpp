#pragma once

#include <QObject>
#include <QTimer>

#include <memory>

class QNetworkReply;

namespace chatterino {

class NetworkData;

}  // namespace chatterino

namespace chatterino::network::detail {

class NetworkTask : public QObject
{
    Q_OBJECT

public:
    NetworkTask(std::shared_ptr<NetworkData> &&data);
    ~NetworkTask() override;

    NetworkTask(const NetworkTask &) = delete;
    NetworkTask(NetworkTask &&) = delete;
    NetworkTask &operator=(const NetworkTask &) = delete;
    NetworkTask &operator=(NetworkTask &&) = delete;

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public Q_SLOTS:
    void run();

private:
    QNetworkReply *createReply();

    void logReply();
    void writeToCache(const QByteArray &bytes) const;

    std::shared_ptr<NetworkData> data_;
    QNetworkReply *reply_{};  // parent: default (accessManager)
    QTimer *timer_{};         // parent: this

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
private Q_SLOTS:
    void timeout();
    void finished();
};

}  // namespace chatterino::network::detail
