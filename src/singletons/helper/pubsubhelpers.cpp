#include "singletons/helper/pubsubhelpers.hpp"

#include "singletons/accountmanager.hpp"
#include "singletons/helper/pubsubactions.hpp"
#include "util/rapidjson-helpers.hpp"

namespace chatterino {
namespace singletons {

const rapidjson::Value &getArgs(const rapidjson::Value &data)
{
    if (!data.HasMember("args")) {
        throw std::runtime_error("Missing member args");
    }

    const auto &args = data["args"];

    if (!args.IsArray()) {
        throw std::runtime_error("args must be an array");
    }

    return args;
}

bool getCreatedByUser(const rapidjson::Value &data, ActionUser &user)
{
    return rj::getSafe(data, "created_by", user.name) &&
           rj::getSafe(data, "created_by_user_id", user.id);
}

bool getTargetUser(const rapidjson::Value &data, ActionUser &user)
{
    return rj::getSafe(data, "target_user_id", user.id);
}

std::string Stringify(const rapidjson::Value &v)
{
    return pajlada::Settings::SettingManager::stringify(v);
}

rapidjson::Document CreateListenMessage(const std::vector<std::string> &topicsVec,
                                        std::shared_ptr<providers::twitch::TwitchAccount> account)
{
    rapidjson::Document msg(rapidjson::kObjectType);
    auto &a = msg.GetAllocator();

    rj::set(msg, "type", "LISTEN");

    rapidjson::Value data(rapidjson::kObjectType);

    if (account) {
        rj::set(data, "auth_token", account->getOAuthToken(), a);
    }

    rapidjson::Value topics(rapidjson::kArrayType);
    for (const auto &topic : topicsVec) {
        rj::add(topics, topic, a);
    }

    rj::set(data, "topics", topics, a);

    rj::set(msg, "data", data);

    return msg;
}

rapidjson::Document CreateUnlistenMessage(const std::vector<std::string> &topicsVec)
{
    rapidjson::Document msg(rapidjson::kObjectType);
    auto &a = msg.GetAllocator();

    rj::set(msg, "type", "UNLISTEN");

    auto &accountManager = AccountManager::getInstance();

    rapidjson::Value data(rapidjson::kObjectType);

    rapidjson::Value topics(rapidjson::kArrayType);
    for (const auto &topic : topicsVec) {
        rj::add(topics, topic, a);
    }

    rj::set(data, "topics", topics, a);

    rj::set(msg, "data", data);

    return msg;
}

}  // namespace singletons
}  // namespace chatterino
