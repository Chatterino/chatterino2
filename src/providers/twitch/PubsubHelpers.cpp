#include "providers/twitch/PubsubHelpers.hpp"

#include "providers/twitch/PubsubActions.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

rapidjson::Document createUnlistenMessage(const std::vector<QString> &topicsVec)
{
    rapidjson::Document msg(rapidjson::kObjectType);
    auto &a = msg.GetAllocator();

    rj::set(msg, "type", "UNLISTEN");

    rapidjson::Value data(rapidjson::kObjectType);

    rapidjson::Value topics(rapidjson::kArrayType);
    for (const auto &topic : topicsVec)
    {
        rj::add(topics, topic, a);
    }

    rj::set(data, "topics", topics, a);

    rj::set(msg, "data", data);

    return msg;
}

}  // namespace chatterino
