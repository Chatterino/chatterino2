#include "providers/twitch/TwitchUser.hpp"

#include "util/RapidjsonHelpers.hpp"

namespace chatterino {
namespace providers {
namespace twitch {

TwitchUser TwitchUser::fromJSON(const rapidjson::Value &value)
{
    TwitchUser user;

    if (!value.IsObject()) {
        throw std::runtime_error("JSON value is not an object");
    }

    if (!rj::getSafe(value, "_id", user.id)) {
        throw std::runtime_error("Missing ID key");
    }

    if (!rj::getSafe(value, "name", user.name)) {
        throw std::runtime_error("Missing name key");
    }

    if (!rj::getSafe(value, "display_name", user.displayName)) {
        throw std::runtime_error("Missing display name key");
    }

    return user;
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
