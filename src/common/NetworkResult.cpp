#include "common/NetworkResult.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <QJsonDocument>
#include "qlogging.hpp"

namespace chatterino {

NetworkResult::NetworkResult(const QByteArray &data, int status)
    : data_(data)
    , status_(status)
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
        qCDebug(chatterinoCommon) << "JSON parse error:"
                 << rapidjson::GetParseError_En(result.Code()) << "("
                 << result.Offset() << ")";
        return ret;
    }

    return ret;
}

const QByteArray &NetworkResult::getData() const
{
    return this->data_;
}

int NetworkResult::status() const
{
    return this->status_;
}

}  // namespace chatterino
