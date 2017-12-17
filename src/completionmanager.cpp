#include "completionmanager.hpp"
#include "common.hpp"
#include "debug/log.hpp"
#include "emotemanager.hpp"

namespace chatterino {

CompletionModel::CompletionModel(const std::string &_channelName)
    : channelName(_channelName)
{
}

void CompletionModel::refresh()
{
    // debug::Log("[CompletionModel:{}] Refreshing...]", this->channelName);

    auto &emoteManager = EmoteManager::getInstance();
    this->emotes.clear();

    // User-specific: Twitch Emotes
    // TODO: Fix this so it properly updates with the proper api. oauth token needs proper scope
    for (const auto &m : emoteManager.twitchAccountEmotes) {
        for (const auto &emoteName : m.second.emoteCodes) {
            this->addString(emoteName);
        }
    }

    // Global: BTTV Global Emotes
    std::vector<std::string> &bttvGlobalEmoteCodes = emoteManager.bttvGlobalEmoteCodes;
    for (const auto &m : bttvGlobalEmoteCodes) {
        this->addString(m);
    }

    // Global: FFZ Global Emotes
    std::vector<std::string> &ffzGlobalEmoteCodes = emoteManager.ffzGlobalEmoteCodes;
    for (const auto &m : ffzGlobalEmoteCodes) {
        this->addString(m);
    }

    // Channel-specific: BTTV Channel Emotes
    std::vector<std::string> &bttvChannelEmoteCodes =
        emoteManager.bttvChannelEmoteCodes[this->channelName];
    for (const auto &m : bttvChannelEmoteCodes) {
        this->addString(m);
    }

    // Channel-specific: FFZ Channel Emotes
    std::vector<std::string> &ffzChannelEmoteCodes =
        emoteManager.ffzChannelEmoteCodes[this->channelName];
    for (const auto &m : ffzChannelEmoteCodes) {
        this->addString(m);
    }

    // Global: Emojis
    const auto &emojiShortCodes = emoteManager.emojiShortCodes;
    for (const auto &m : emojiShortCodes) {
        this->addString(":" + m + ":");
    }

    // TODO: Add Channel-specific: Usernames
}

void CompletionModel::addString(const std::string &str)
{
    // Always add a space at the end of completions
    this->emotes.push_back(qS(str) + " ");
}

CompletionModel *CompletionManager::createModel(const std::string &channelName)
{
    auto it = this->models.find(channelName);
    if (it != this->models.end()) {
        return it->second;
    }

    CompletionModel *ret = new CompletionModel(channelName);
    this->models[channelName] = ret;

    return ret;
}

}  // namespace chatterino
