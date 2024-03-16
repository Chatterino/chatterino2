#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <rapidjson/document.h>

#include <optional>

namespace chatterino {

class NetworkResult
{
public:
    using NetworkError = QNetworkReply::NetworkError;

    NetworkResult(NetworkError error, const QVariant &httpStatusCode,
                  QByteArray data);

    /// Parses the result as json and returns the root as an object.
    /// Returns empty object if parsing failed.
    QJsonObject parseJson() const;
    /// Parses the result as json and returns the root as an array.
    /// Returns empty object if parsing failed.
    QJsonArray parseJsonArray() const;
    /// Parses the result as json and returns the document.
    rapidjson::Document parseRapidJson() const;
    const QByteArray &getData() const;

    /// The error code of the reply.
    /// In case of a successful reply, this will be NoError (0)
    NetworkError error() const
    {
        return this->error_;
    }

    /// The HTTP status code if a response was received.
    std::optional<int> status() const
    {
        return this->status_;
    }

    /// Formats the error.
    /// If a reply is received, returns the HTTP status otherwise, the network error.
    QString formatError() const;

private:
    QByteArray data_;

    NetworkError error_;
    std::optional<int> status_;
};

}  // namespace chatterino
