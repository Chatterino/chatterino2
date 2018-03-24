#include "completionmodel.hpp"

#include "common.hpp"
#include "debug/log.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/completionmanager.hpp"
#include "singletons/emotemanager.hpp"

#include <QtAlgorithms>

namespace chatterino {
namespace singletons {
CompletionModel::CompletionModel(const QString &_channelName)
    : channelName(_channelName)
{
}

void CompletionModel::refresh()
{
    // debug::Log("[CompletionModel:{}] Refreshing...]", this->channelName);

    auto &emoteManager = singletons::EmoteManager::getInstance();

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
        emoteManager.bttvChannelEmoteCodes[this->channelName.toStdString()];
    for (const auto &m : bttvChannelEmoteCodes) {
        this->addString(m);
    }

    // Channel-specific: FFZ Channel Emotes
    std::vector<std::string> &ffzChannelEmoteCodes =
        emoteManager.ffzChannelEmoteCodes[this->channelName.toStdString()];
    for (const auto &m : ffzChannelEmoteCodes) {
        this->addString(m);
    }

    // Global: Emojis
    const auto &emojiShortCodes = emoteManager.emojiShortCodes;
    for (const auto &m : emojiShortCodes) {
        this->addString(":" + m + ":");
    }

    // Channel-specific: Usernames
    // fourtf: only works with twitch chat
    //    auto c = singletons::ChannelManager::getInstance().getTwitchChannel(this->channelName);
    //    auto usernames = c->getUsernamesForCompletions();
    //    for (const auto &name : usernames) {
    //        assert(!name.displayName.isEmpty());
    //        this->addString(name.displayName);
    //        this->addString('@' + name.displayName);

    //        if (!name.localizedName.isEmpty()) {
    //            this->addString(name.localizedName);
    //            this->addString('@' + name.localizedName);
    //        }
    //    }
}

void CompletionModel::addString(const std::string &str)
{
    // Always add a space at the end of completions
    this->emotes.insert(this->createEmote(str + " "));
}

void CompletionModel::addString(const QString &str)
{
    // Always add a space at the end of completions
    this->emotes.insert(this->createEmote(str + " "));
}

void CompletionModel::addUser(const QString &str)
{
    // Always add a space at the end of completions
    this->emotes.insert(this->createUser(str + " "));
}
}  // namespace singletons
}  // namespace chatterino
