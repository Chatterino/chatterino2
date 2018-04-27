#include "util/completionmodel.hpp"

#include "application.hpp"
#include "common.hpp"
#include "debug/log.hpp"
#include "singletons/emotemanager.hpp"

#include <QtAlgorithms>

#include <utility>

namespace chatterino {

CompletionModel::CompletionModel(const QString &_channelName)
    : channelName(_channelName)
{
}

void CompletionModel::refresh()
{
    debug::Log("[CompletionModel:{}] Refreshing...]", this->channelName);

    auto app = getApp();

    // User-specific: Twitch Emotes
    // TODO: Fix this so it properly updates with the proper api. oauth token needs proper scope
    for (const auto &m : app->emotes->twitchAccountEmotes) {
        for (const auto &emoteName : m.second.emoteCodes) {
            // XXX: No way to discern between a twitch global emote and sub emote right now
            this->addString(emoteName, TaggedString::Type::TwitchGlobalEmote);
        }
    }

    // Global: BTTV Global Emotes
    std::vector<std::string> &bttvGlobalEmoteCodes = app->emotes->bttvGlobalEmoteCodes;
    for (const auto &m : bttvGlobalEmoteCodes) {
        this->addString(m, TaggedString::Type::BTTVGlobalEmote);
    }

    // Global: FFZ Global Emotes
    std::vector<std::string> &ffzGlobalEmoteCodes = app->emotes->ffzGlobalEmoteCodes;
    for (const auto &m : ffzGlobalEmoteCodes) {
        this->addString(m, TaggedString::Type::FFZGlobalEmote);
    }

    // Channel-specific: BTTV Channel Emotes
    std::vector<std::string> &bttvChannelEmoteCodes =
        app->emotes->bttvChannelEmoteCodes[this->channelName.toStdString()];
    for (const auto &m : bttvChannelEmoteCodes) {
        this->addString(m, TaggedString::Type::BTTVChannelEmote);
    }

    // Channel-specific: FFZ Channel Emotes
    std::vector<std::string> &ffzChannelEmoteCodes =
        app->emotes->ffzChannelEmoteCodes[this->channelName.toStdString()];
    for (const auto &m : ffzChannelEmoteCodes) {
        this->addString(m, TaggedString::Type::FFZChannelEmote);
    }

    // Global: Emojis
    const auto &emojiShortCodes = app->emotes->emojiShortCodes;
    for (const auto &m : emojiShortCodes) {
        this->addString(":" + m + ":", TaggedString::Type::Emoji);
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

void CompletionModel::addString(const std::string &str, TaggedString::Type type)
{
    this->addString(qS(str), type);
}

void CompletionModel::addString(const QString &str, TaggedString::Type type)
{
    std::lock_guard<std::mutex> lock(this->emotesMutex);

    // Always add a space at the end of completions
    this->emotes.insert({str + " ", type});
}

void CompletionModel::addUser(const QString &str)
{
    auto ts = this->createUser(str + " ");
    // Always add a space at the end of completions
    std::pair<std::set<TaggedString>::iterator, bool> p = this->emotes.insert(ts);
    if (!p.second) {
        // No inseration was made, figure out if we need to replace the username.

        if (p.first->str > ts.str) {
            // Replace lowercase version of name with mixed-case version
            this->emotes.erase(p.first);
            auto result2 = this->emotes.insert(ts);
            assert(result2.second);
        } else {
            p.first->timeAdded = std::chrono::steady_clock::now();
        }
    }
}

void CompletionModel::ClearExpiredStrings()
{
    std::lock_guard<std::mutex> lock(this->emotesMutex);

    auto now = std::chrono::steady_clock::now();

    for (auto it = this->emotes.begin(); it != this->emotes.end();) {
        const auto &taggedString = *it;

        if (taggedString.HasExpired(now)) {
            // debug::Log("String {} expired", taggedString.str);
            it = this->emotes.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace chatterino
