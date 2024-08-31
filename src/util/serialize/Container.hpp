#pragma once

#include <pajlada/serialize.hpp>
#include <QString>

#include <unordered_map>

namespace pajlada {

template <typename ValueType, typename RJValue>
struct Serialize<std::unordered_map<QString, ValueType>, RJValue> {
    static RJValue get(const std::unordered_map<QString, ValueType> &value,
                       typename RJValue::AllocatorType &a)
    {
        RJValue ret(rapidjson::kObjectType);

        for (auto it = value.begin(); it != value.end(); ++it)
        {
            detail::AddMember<ValueType, RJValue>(ret, it->first.toUtf8(),
                                                  it->second, a);
        }

        return ret;
    }
};

template <typename ValueType, typename RJValue>
struct Deserialize<std::unordered_map<QString, ValueType>, RJValue> {
    static std::unordered_map<QString, ValueType> get(const RJValue &value,
                                                      bool *error = nullptr)
    {
        std::unordered_map<QString, ValueType> ret;

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return ret;
        }

        for (typename RJValue::ConstMemberIterator it = value.MemberBegin();
             it != value.MemberEnd(); ++it)
        {
            ret.emplace(it->name.GetString(),
                        Deserialize<ValueType, RJValue>::get(it->value, error));
        }

        return ret;
    }
};

}  // namespace pajlada
