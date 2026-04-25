// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/RapidjsonHelpers.hpp"

#include <rapidjson/prettywriter.h>

namespace chatterino {
namespace rj {

void addMember(rapidjson::Value &obj, const char *key, rapidjson::Value &&value,
               rapidjson::Document::AllocatorType &a)
{
    obj.AddMember(rapidjson::Value(key, a).Move(), value, a);
}

void addMember(rapidjson::Value &obj, const char *key, rapidjson::Value &value,
               rapidjson::Document::AllocatorType &a)
{
    obj.AddMember(rapidjson::Value(key, a).Move(), value.Move(), a);
}

void setOptionally(rapidjson::Value &obj, const char *key, const QString &value,
                   rapidjson::Document::AllocatorType &a)
{
    assert(obj.IsObject());

    if (!value.isNull())
    {
        addMember(obj, key, pajlada::Serialize<QString>::get(value, a), a);
    }
}

QString stringify(const rapidjson::Value &value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);

    return buffer.GetString();
}

bool getSafeObject(rapidjson::Value &obj, const char *key,
                   rapidjson::Value &out)
{
    if (!checkJsonValue(obj, key))
    {
        return false;
    }

    out = obj[key].Move();
    return true;
}

bool checkJsonValue(const rapidjson::Value &obj, const char *key)
{
    return obj.IsObject() && !obj.IsNull() && obj.HasMember(key);
}

}  // namespace rj
}  // namespace chatterino
