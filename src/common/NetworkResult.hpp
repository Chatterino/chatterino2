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
    using Error = QNetworkReply::NetworkError;

    NetworkResult(Error error, const QVariant &httpStatusCode, QByteArray data);

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
    Error error() const
    {
        return this->error_;
    }

    /// The HTTP status code if a request was made.
    std::optional<int> status() const
    {
        return this->status_;
    }

    /// Formats the error:
    /// without HTTP status: [Name]
    /// with HTTP status: [HTTP status]
    QString formatError() const;

private:
    QByteArray data_;

    Error error_;
    std::optional<int> status_;
};

}  // namespace chatterino
