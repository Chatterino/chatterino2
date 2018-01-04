#pragma once

#include "util/serialize-custom.hpp"

#include <QString>
#include <pajlada/settings/serialize.hpp>

namespace chatterino {
namespace messages {

struct HighlightPhrase {
    QString key;
    bool sound;
    bool alert;

    bool operator==(const HighlightPhrase &rhs) const
    {
        return std::tie(this->key, this->sound, this->alert) ==
               std::tie(rhs.key, rhs.sound, rhs.alert);
    }
};
}  // namespace messages
}  // namespace chatterino

namespace pajlada {
namespace Settings {

template <>
struct Serialize<chatterino::messages::HighlightPhrase> {
    static rapidjson::Value get(const chatterino::messages::HighlightPhrase &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        AddMember(ret, "key", value.key, a);
        AddMember(ret, "alert", value.alert, a);
        AddMember(ret, "sound", value.sound, a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::messages::HighlightPhrase> {
    static chatterino::messages::HighlightPhrase get(const rapidjson::Value &value)
    {
        chatterino::messages::HighlightPhrase ret;
        if (!value.IsObject()) {
            return ret;
        }

        if (value.HasMember("key")) {
            const rapidjson::Value &key = value["key"];
            if (key.IsString()) {
                ret.key = key.GetString();
            }
        }

        if (value.HasMember("alert")) {
            const rapidjson::Value &alert = value["alert"];
            if (alert.IsBool()) {
                ret.alert = alert.GetBool();
            }
        }

        if (value.HasMember("sound")) {
            const rapidjson::Value &sound = value["sound"];
            if (sound.IsBool()) {
                ret.sound = sound.GetBool();
            }
        }

        return ret;
    }
};

}  // namespace Settings
}  // namespace pajlada
