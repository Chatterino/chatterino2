#pragma once

#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <rapidjson/document.h>

#include <cassert>
#include <string>

namespace chatterino {
namespace rj {

    void addMember(rapidjson::Value &obj, const char *key,
                   rapidjson::Value &&value,
                   rapidjson::Document::AllocatorType &a);
    void addMember(rapidjson::Value &obj, const char *key,
                   rapidjson::Value &value,
                   rapidjson::Document::AllocatorType &a);

    template <typename Type>
    void set(rapidjson::Value &obj, const char *key, const Type &value,
             rapidjson::Document::AllocatorType &a)
    {
        assert(obj.IsObject());

        addMember(obj, key, pajlada::Serialize<Type>::get(value, a), a);
    }

    template <>
    inline void set(rapidjson::Value &obj, const char *key,
                    const rapidjson::Value &value,
                    rapidjson::Document::AllocatorType &a)
    {
        assert(obj.IsObject());

        addMember(obj, key, const_cast<rapidjson::Value &>(value), a);
    }

    template <typename Type>
    void set(rapidjson::Document &obj, const char *key, const Type &value)
    {
        assert(obj.IsObject());

        auto &a = obj.GetAllocator();

        addMember(obj, key, pajlada::Serialize<Type>::get(value, a), a);
    }

    template <>
    inline void set(rapidjson::Document &obj, const char *key,
                    const rapidjson::Value &value)
    {
        assert(obj.IsObject());

        auto &a = obj.GetAllocator();

        addMember(obj, key, const_cast<rapidjson::Value &>(value), a);
    }

    template <typename Type>
    void add(rapidjson::Value &arr, const Type &value,
             rapidjson::Document::AllocatorType &a)
    {
        assert(arr.IsArray());

        arr.PushBack(pajlada::Serialize<Type>::get(value, a), a);
    }

    bool checkJsonValue(const rapidjson::Value &obj, const char *key);

    template <typename Type>
    bool getSafe(const rapidjson::Value &obj, const char *key, Type &out)
    {
        if (!checkJsonValue(obj, key))
        {
            return false;
        }

        bool error = false;
        out = pajlada::Deserialize<Type>::get(obj[key], &error);

        return !error;
    }

    template <typename Type>
    bool getSafe(const rapidjson::Value &value, Type &out)
    {
        bool error = false;
        out = pajlada::Deserialize<Type>::get(value, &error);

        return !error;
    }

    bool getSafeObject(rapidjson::Value &obj, const char *key,
                       rapidjson::Value &out);

    QString stringify(const rapidjson::Value &value);

}  // namespace rj
}  // namespace chatterino
