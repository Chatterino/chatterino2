#include "util/rapidjson-helpers.hpp"

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

}  // namespace rj
}  // namespace chatterino
