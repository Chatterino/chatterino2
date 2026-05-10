#pragma once

#include "controllers/highlights/types/Common.hpp"

#include "util/RapidJsonSerializeQString.hpp"

#include <QUuid>

#include <cassert>

namespace chatterino::highlights {

bool matchesType(const rapidjson::Value &obj, QStringView expectedType)
{
    assert(obj.IsObject());

    if (!obj.IsObject())
    {
        return false;
    }

    auto member = obj.FindMember("type");
    if (member == obj.MemberEnd())
    {
        return false;
    }

    QString actualType =
        pajlada::Deserialize<QString>::get(member->value, nullptr);

    return actualType == expectedType;
}

bool matchesID(const rapidjson::Value &obj, QStringView expectedID)
{
    assert(obj.IsObject());

    if (!obj.IsObject())
    {
        return false;
    }

    auto member = obj.FindMember("id");
    if (member == obj.MemberEnd())
    {
        return false;
    }

    QString actualID =
        pajlada::Deserialize<QString>::get(member->value, nullptr);

    return actualID == expectedID;
}

QString generateID()
{
    return QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
}

}  // namespace chatterino::highlights
