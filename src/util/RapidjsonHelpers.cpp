#include "util/rapidjson-helpers.hpp"

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

std::string stringify(const rapidjson::Value &value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);

    return std::string(buffer.GetString());
}

}  // namespace rj
}  // namespace chatterino
