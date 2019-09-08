#include "common/NetworkResult.hpp"

#include "debug/Log.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <QJsonDocument>

namespace chatterino {

NetworkResult::NetworkResult(const QByteArray &data)
    : data_(data)
{
}

QJsonObject NetworkResult::parseJson() const
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(this->data_));
    if (jsonDoc.isNull())
    {
        return QJsonObject{};
    }

    return jsonDoc.object();
}

QJsonArray NetworkResult::parseJsonArray() const
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(this->data_));
    if (jsonDoc.isNull())
    {
        return QJsonArray{};
    }

    return jsonDoc.array();
}

rapidjson::Document NetworkResult::parseRapidJson() const
{
    rapidjson::Document ret(rapidjson::kObjectType);

    rapidjson::ParseResult result =
        ret.Parse(this->data_.data(), this->data_.length());

    if (result.Code() != rapidjson::kParseErrorNone)
    {
        log("JSON parse error: {} ({})",
            rapidjson::GetParseError_En(result.Code()), result.Offset());
        return ret;
    }

    return ret;
}

const QByteArray &NetworkResult::getData() const
{
    return this->data_;
}

}  // namespace chatterino
