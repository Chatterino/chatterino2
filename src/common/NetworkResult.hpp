#pragma once

#include <rapidjson/document.h>
#include <QJsonObject>

namespace chatterino {

class NetworkResult
{
public:
    NetworkResult(const QByteArray &data, int status);

    /// Parses the result as json and returns the root as an object.
    /// Returns empty object if parsing failed.
    QJsonObject parseJson() const;
    /// Parses the result as json and returns the root as an array.
    /// Returns empty object if parsing failed.
    QJsonArray parseJsonArray() const;
    /// Parses the result as json and returns the document.
    rapidjson::Document parseRapidJson() const;
    const QByteArray &getData() const;
    int status() const;

    static constexpr int timedoutStatus = -2;

private:
    int status_;
    QByteArray data_;
};

}  // namespace chatterino
