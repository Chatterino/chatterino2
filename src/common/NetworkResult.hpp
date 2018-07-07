#pragma once

#include <rapidjson/document.h>
#include <QJsonObject>

namespace chatterino {

class NetworkResult
{
    QByteArray data_;

public:
    NetworkResult(const QByteArray &data);

    QJsonObject parseJson() const;
    rapidjson::Document parseRapidJson() const;
    QByteArray getData() const;
};

}  // namespace chatterino
