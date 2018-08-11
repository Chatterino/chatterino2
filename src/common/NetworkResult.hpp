#pragma once

#include <rapidjson/document.h>
#include <QJsonObject>

namespace chatterino {

class NetworkResult
{
public:
    NetworkResult(const QByteArray &data);

    QJsonObject parseJson() const;
    rapidjson::Document parseRapidJson() const;
    const QByteArray &getData() const;

private:
    QByteArray data_;
};

}  // namespace chatterino
