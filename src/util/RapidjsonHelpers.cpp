#include "util/RapidjsonHelpers.hpp"

#include <rapidjson/prettywriter.h>

namespace chatterino {
namespace rj {

    void addMember(rapidjson::Value &obj, const char *key,
                   rapidjson::Value &&value,
                   rapidjson::Document::AllocatorType &a)
    {
        obj.AddMember(rapidjson::Value(key, a).Move(), value, a);
    }

    void addMember(rapidjson::Value &obj, const char *key,
                   rapidjson::Value &value,
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
