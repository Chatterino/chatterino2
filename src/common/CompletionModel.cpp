#include "common/CompletionModel.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "controllers/commands/CommandController.hpp"
#include "debug/Log.hpp"
#include "singletons/Emotes.hpp"

#include <QtAlgorithms>

#include <utility>

namespace chatterino {

// -- TaggedString

CompletionModel::TaggedString::TaggedString(const QString &_str, Type _type)
    : str(_str)
    , type(_type)
    , timeAdded(std::chrono::steady_clock::now())
{
}

bool CompletionModel::TaggedString::isExpired(
    const std::chrono::steady_clock::time_point &now) const
{
    switch (this->type) {
        case Type::Username: {
            static std::chrono::minutes expirationTimer(10);

            return (this->timeAdded + expirationTimer < now);
        } break;

        default: {
            return false;
        } break;
    }

    return false;
}

bool CompletionModel::TaggedString::isEmote() const
{
    return this->type > Type::EmoteStart && this->type < Type::EmoteEnd;
}

bool CompletionModel::TaggedString::operator<(const TaggedString &that) const
{
    if (this->isEmote()) {
        if (that.isEmote()) {
            int k = QString::compare(this->str, that.str, Qt::CaseInsensitive);
            if (k == 0) {
                return this->str > that.str;
            }

            return k < 0;
        }

        return true;
    }

    if (that.isEmote()) {
        return false;
    }

    int k = QString::compare(this->str, that.str, Qt::CaseInsensitive);
    if (k == 0) {
        return false;
    }

    return k < 0;
}

// -- CompletionModel

CompletionModel::CompletionModel(const QString &channelName)
    : channelName_(channelName)
{
}

int CompletionModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant CompletionModel::data(const QModelIndex &index, int) const
{
    std::lock_guard<std::mutex> lock(this->emotesMutex_);

    // TODO: Implement more safely
    auto it = this->emotes_.begin();
    std::advance(it, index.row());
    return QVariant(it->str);
}

int CompletionModel::rowCount(const QModelIndex &) const
{
    std::lock_guard<std::mutex> lock(this->emotesMutex_);

    return this->emotes_.size();
}

void CompletionModel::refresh()
{
    Log("[CompletionModel:{}] Refreshing...]", this->channelName_);

    auto app = getApp();

    // User-specific: Twitch Emotes
    // TODO: Fix this so it properly updates with the proper api. oauth token needs proper scope
    for (const auto &m : app->emotes->twitch.emotes) {
        for (const auto &emoteName : m.second.emoteCodes) {
            // XXX: No way to discern between a twitch global emote and sub emote right now
            this->addString(emoteName, TaggedString::Type::TwitchGlobalEmote);
        }
    }

    // Global: BTTV Global Emotes
    std::vector<QString> &bttvGlobalEmoteCodes = app->emotes->bttv.globalEmoteCodes;
    for (const auto &m : bttvGlobalEmoteCodes) {
        this->addString(m, TaggedString::Type::BTTVGlobalEmote);
    }

    // Global: FFZ Global Emotes
    std::vector<QString> &ffzGlobalEmoteCodes = app->emotes->ffz.globalEmoteCodes;
    for (const auto &m : ffzGlobalEmoteCodes) {
        this->addString(m, TaggedString::Type::FFZGlobalEmote);
    }

    // Channel-specific: BTTV Channel Emotes
    std::vector<QString> &bttvChannelEmoteCodes =
        app->emotes->bttv.channelEmoteCodes[this->channelName_];
    for (const auto &m : bttvChannelEmoteCodes) {
        this->addString(m, TaggedString::Type::BTTVChannelEmote);
    }

    // Channel-specific: FFZ Channel Emotes
    std::vector<QString> &ffzChannelEmoteCodes =
        app->emotes->ffz.channelEmoteCodes[this->channelName_];
    for (const auto &m : ffzChannelEmoteCodes) {
        this->addString(m, TaggedString::Type::FFZChannelEmote);
    }

    // Global: Emojis
    const auto &emojiShortCodes = app->emotes->emojis.shortCodes;
    for (const auto &m : emojiShortCodes) {
        this->addString(":" + m + ":", TaggedString::Type::Emoji);
    }

    // Commands
    for (auto &command : app->commands->items.getVector()) {
        this->addString(command.name, TaggedString::Command);
    }

    for (auto &command : app->commands->getDefaultTwitchCommandList()) {
        this->addString(command, TaggedString::Command);
    }

    // Channel-specific: Usernames
    // fourtf: only works with twitch chat
    //    auto c = ChannelManager::getInstance().getTwitchChannel(this->channelName);
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

void CompletionModel::addString(const QString &str, TaggedString::Type type)
{
    std::lock_guard<std::mutex> lock(this->emotesMutex_);

    // Always add a space at the end of completions
    this->emotes_.insert({str + " ", type});
}

void CompletionModel::addUser(const QString &username)
{
    auto add = [this](const QString &str) {
        auto ts = this->createUser(str + " ");
        // Always add a space at the end of completions
        std::pair<std::set<TaggedString>::iterator, bool> p = this->emotes_.insert(ts);
        if (!p.second) {
            // No inseration was made, figure out if we need to replace the username.

            if (p.first->str > ts.str) {
                // Replace lowercase version of name with mixed-case version
                this->emotes_.erase(p.first);
                auto result2 = this->emotes_.insert(ts);
                assert(result2.second);
            } else {
                p.first->timeAdded = std::chrono::steady_clock::now();
            }
        }
    };

    add(username);
    add("@" + username);
}

void CompletionModel::clearExpiredStrings()
{
    std::lock_guard<std::mutex> lock(this->emotesMutex_);

    auto now = std::chrono::steady_clock::now();

    for (auto it = this->emotes_.begin(); it != this->emotes_.end();) {
        const auto &taggedString = *it;

        if (taggedString.isExpired(now)) {
            // Log("String {} expired", taggedString.str);
            it = this->emotes_.erase(it);
        } else {
            ++it;
        }
    }
}

CompletionModel::TaggedString CompletionModel::createUser(const QString &str)
{
    return TaggedString{str, TaggedString::Type::Username};
}

}  // namespace chatterino
