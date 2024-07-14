#include "common/network/NetworkResult.hpp"

#include "common/QLogging.hpp"

#include <QJsonDocument>
#include <QMetaEnum>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace chatterino {

NetworkResult::NetworkResult(NetworkError error, const QVariant &httpStatusCode,
                             QByteArray data)
    : data_(std::move(data))
    , error_(error)
{
    if (httpStatusCode.isValid())
    {
        this->status_ = httpStatusCode.toInt();
    }
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
        qCWarning(chatterinoCommon)
            << "JSON parse error:" << rapidjson::GetParseError_En(result.Code())
            << "(" << result.Offset() << ")";
        return ret;
    }

    return ret;
}

const QByteArray &NetworkResult::getData() const
{
    return this->data_;
}

QString NetworkResult::formatError() const
{
    // Print the status for errors that mirror HTTP status codes (=0 || >99)
    if (this->status_ && (this->error_ == QNetworkReply::NoError ||
                          this->error_ > QNetworkReply::UnknownNetworkError))
    {
        return QString::number(*this->status_);
    }

    const auto *name =
        QMetaEnum::fromType<QNetworkReply::NetworkError>().valueToKey(
            this->error_);
    if (name == nullptr)
    {
        if (this->status_)
        {
            return QStringLiteral("unknown error (status: %1, error: %2)")
                .arg(QString::number(*this->status_),
                     QString::number(this->error_));
        }

        return QStringLiteral("unknown error (%1)").arg(this->error_);
    }
    return name;
}

}  // namespace chatterino
