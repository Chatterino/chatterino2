#include "TwitchParseCheerEmotes.hpp"

#include <rapidjson/document.h>
#include <QString>
#include <vector>

namespace chatterino {

namespace {

template <typename Type>
inline bool ReadValue(const rapidjson::Value &object, const char *key,
                      Type &out)
{
    if (!object.HasMember(key)) {
        return false;
    }

    const auto &value = object[key];

    if (!value.Is<Type>()) {
        return false;
    }

    out = value.Get<Type>();

    return true;
}

template <>
inline bool ReadValue<QString>(const rapidjson::Value &object, const char *key,
                               QString &out)
{
    if (!object.HasMember(key)) {
        return false;
    }

    const auto &value = object[key];

    if (!value.IsString()) {
        return false;
    }

    out = value.GetString();

    return true;
}

template <>
inline bool ReadValue<std::vector<QString>>(const rapidjson::Value &object,
                                            const char *key,
                                            std::vector<QString> &out)
{
    if (!object.HasMember(key)) {
        return false;
    }

    const auto &value = object[key];

    if (!value.IsArray()) {
        return false;
    }

    for (const rapidjson::Value &innerValue : value.GetArray()) {
        if (!innerValue.IsString()) {
            return false;
        }

        out.emplace_back(innerValue.GetString());
    }

    return true;
}

// Parse a single cheermote set (or "action") from the twitch api
inline bool ParseSingleCheermoteSet(JSONCheermoteSet &set,
                                    const rapidjson::Value &action)
{
    if (!action.IsObject()) {
        return false;
    }

    if (!ReadValue(action, "prefix", set.prefix)) {
        return false;
    }

    if (!ReadValue(action, "scales", set.scales)) {
        return false;
    }

    if (!ReadValue(action, "backgrounds", set.backgrounds)) {
        return false;
    }

    if (!ReadValue(action, "states", set.states)) {
        return false;
    }

    if (!ReadValue(action, "type", set.type)) {
        return false;
    }

    if (!ReadValue(action, "updated_at", set.updatedAt)) {
        return false;
    }

    if (!ReadValue(action, "priority", set.priority)) {
        return false;
    }

    // Tiers
    if (!action.HasMember("tiers")) {
        return false;
    }

    const auto &tiersValue = action["tiers"];

    if (!tiersValue.IsArray()) {
        return false;
    }

    for (const rapidjson::Value &tierValue : tiersValue.GetArray()) {
        JSONCheermoteSet::CheermoteTier tier;

        if (!tierValue.IsObject()) {
            return false;
        }

        if (!ReadValue(tierValue, "min_bits", tier.minBits)) {
            return false;
        }

        if (!ReadValue(tierValue, "id", tier.id)) {
            return false;
        }

        if (!ReadValue(tierValue, "color", tier.color)) {
            return false;
        }

        // Images
        if (!tierValue.HasMember("images")) {
            return false;
        }

        const auto &imagesValue = tierValue["images"];

        if (!imagesValue.IsObject()) {
            return false;
        }

        // Read images object
        for (const auto &imageBackgroundValue : imagesValue.GetObject()) {
            QString background = imageBackgroundValue.name.GetString();
            bool backgroundExists = false;
            for (const auto &bg : set.backgrounds) {
                if (background == bg) {
                    backgroundExists = true;
                    break;
                }
            }

            if (!backgroundExists) {
                continue;
            }

            const rapidjson::Value &imageBackgroundStates =
                imageBackgroundValue.value;
            if (!imageBackgroundStates.IsObject()) {
                continue;
            }

            // Read each key which represents a background
            for (const auto &imageBackgroundState :
                 imageBackgroundStates.GetObject()) {
                QString state = imageBackgroundState.name.GetString();
                bool stateExists = false;
                for (const auto &_state : set.states) {
                    if (state == _state) {
                        stateExists = true;
                        break;
                    }
                }

                if (!stateExists) {
                    continue;
                }

                const rapidjson::Value &imageScalesValue =
                    imageBackgroundState.value;
                if (!imageScalesValue.IsObject()) {
                    continue;
                }

                // Read each key which represents a scale
                for (const auto &imageScaleValue :
                     imageScalesValue.GetObject()) {
                    QString scale = imageScaleValue.name.GetString();
                    bool scaleExists = false;
                    for (const auto &_scale : set.scales) {
                        if (scale == _scale) {
                            scaleExists = true;
                            break;
                        }
                    }

                    if (!scaleExists) {
                        continue;
                    }

                    const rapidjson::Value &imageScaleURLValue =
                        imageScaleValue.value;
                    if (!imageScaleURLValue.IsString()) {
                        continue;
                    }

                    QString url = imageScaleURLValue.GetString();

                    bool ok = false;
                    qreal scaleNumber = scale.toFloat(&ok);
                    if (!ok) {
                        continue;
                    }

                    qreal chatterinoScale = 1 / scaleNumber;

                    auto image = Image::fromUrl({url}, chatterinoScale);

                    // TODO(pajlada): Fill in name and tooltip
                    tier.images[background][state][scale] = image;
                }
            }
        }

        set.tiers.emplace_back(tier);
    }

    return true;
}
}  // namespace

// Look through the results of
// https://api.twitch.tv/kraken/bits/actions?channel_id=11148817 for cheermote
// sets or "Actions" as they are called in the API
std::vector<JSONCheermoteSet> ParseCheermoteSets(const rapidjson::Document &d)
{
    std::vector<JSONCheermoteSet> sets;

    if (!d.IsObject()) {
        return sets;
    }

    if (!d.HasMember("actions")) {
        return sets;
    }

    const auto &actionsValue = d["actions"];

    if (!actionsValue.IsArray()) {
        return sets;
    }

    for (const auto &action : actionsValue.GetArray()) {
        JSONCheermoteSet set;
        bool res = ParseSingleCheermoteSet(set, action);

        if (res) {
            sets.emplace_back(set);
        }
    }

    return sets;
}
}  // namespace chatterino
