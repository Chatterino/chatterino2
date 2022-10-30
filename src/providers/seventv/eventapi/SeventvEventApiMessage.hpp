#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <boost/optional.hpp>
#include <magic_enum.hpp>
#include "providers/seventv/SeventvEventApi.hpp"

namespace chatterino {

struct SeventvEventApiMessage {
    QJsonObject data;

    SeventvEventApiOpcode op;

    SeventvEventApiMessage(QJsonObject _json);

    template <class InnerClass>
    boost::optional<InnerClass> toInner();
};

template <class InnerClass>
boost::optional<InnerClass> SeventvEventApiMessage::toInner()
{
    return InnerClass{this->data};
}

static boost::optional<SeventvEventApiMessage> parseSeventvEventApiBaseMessage(
    const QString &blob)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(blob.toUtf8()));

    if (jsonDoc.isNull())
    {
        return boost::none;
    }

    return SeventvEventApiMessage(jsonDoc.object());
}

}  // namespace chatterino
