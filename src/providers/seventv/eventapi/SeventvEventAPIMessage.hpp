#pragma once

#include "providers/seventv/eventapi/SeventvEventAPISubscription.hpp"

#include <boost/optional.hpp>
#include <magic_enum.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace chatterino {

struct SeventvEventAPIMessage {
    QJsonObject data;

    SeventvEventAPIOpcode op;

    SeventvEventAPIMessage(QJsonObject _json);

    template <class InnerClass>
    boost::optional<InnerClass> toInner();
};

template <class InnerClass>
boost::optional<InnerClass> SeventvEventAPIMessage::toInner()
{
    return InnerClass{this->data};
}

static boost::optional<SeventvEventAPIMessage> parseSeventvEventAPIBaseMessage(
    const QString &blob)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(blob.toUtf8()));

    if (jsonDoc.isNull())
    {
        return boost::none;
    }

    return SeventvEventAPIMessage(jsonDoc.object());
}

}  // namespace chatterino
