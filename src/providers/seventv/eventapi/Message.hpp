#pragma once

#include "providers/seventv/eventapi/Subscription.hpp"

#include <boost/optional.hpp>
#include <magic_enum.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace chatterino::seventv::eventapi {

struct Message {
    QJsonObject data;

    Opcode op;

    Message(QJsonObject _json);

    template <class InnerClass>
    boost::optional<InnerClass> toInner();
};

template <class InnerClass>
boost::optional<InnerClass> Message::toInner()
{
    return InnerClass{this->data};
}

static boost::optional<Message> parseBaseMessage(const QString &blob)
{
    QJsonDocument jsonDoc(QJsonDocument::fromJson(blob.toUtf8()));

    if (jsonDoc.isNull())
    {
        return boost::none;
    }

    return Message(jsonDoc.object());
}

}  // namespace chatterino::seventv::eventapi
