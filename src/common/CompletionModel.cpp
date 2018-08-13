#include "common/CompletionModel.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/UsernameSet.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandController.hpp"
#include "debug/Benchmark.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"
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

void CompletionModel::refresh(const QString &prefix)
{
    {
        std::lock_guard<std::mutex> guard(this->emotesMutex_);
        this->emotes_.clear();
    }

    if (prefix.length() < 2) return;

    BenchmarkGuard guard("CompletionModel::refresh");

    auto addString = [&](const QString &str, TaggedString::Type type) {
        if (str.startsWith(prefix)) this->emotes_.emplace(str + " ", type);
    };

    auto _channel = getApp()->twitch2->getChannelOrEmpty(this->channelName_);

    if (auto channel = dynamic_cast<TwitchChannel *>(_channel.get())) {
        // account emotes
        if (auto account = getApp()->accounts->twitch.getCurrent()) {
            for (const auto &emote : account->accessEmotes()->allEmoteNames) {
                // XXX: No way to discern between a twitch global emote and sub
                // emote right now
                addString(emote.string, TaggedString::Type::TwitchGlobalEmote);
            }
        }

        // Usernames
        if (prefix.length() >= UsernameSet::PrefixLength) {
            auto usernames = channel->accessChatters();

            for (const auto &name : usernames->subrange(Prefix(prefix))) {
                addString(name, TaggedString::Type::Username);
                addString("@" + name, TaggedString::Type::Username);
            }
        }

        // Bttv Global
        for (auto &emote : *channel->globalBttv().emotes()) {
            addString(emote.first.string, TaggedString::Type::BTTVChannelEmote);
        }

        // Ffz Global
        for (auto &emote : *channel->globalFfz().emotes()) {
            addString(emote.first.string, TaggedString::Type::FFZChannelEmote);
        }

        // Bttv Channel
        for (auto &emote : *channel->bttvEmotes()) {
            addString(emote.first.string, TaggedString::Type::BTTVGlobalEmote);
        }

        // Ffz Channel
        for (auto &emote : *channel->ffzEmotes()) {
            addString(emote.first.string, TaggedString::Type::BTTVGlobalEmote);
        }

        // Emojis
        if (prefix.startsWith(":")) {
            const auto &emojiShortCodes = getApp()->emotes->emojis.shortCodes;
            for (auto &m : emojiShortCodes) {
                addString(":" + m + ":", TaggedString::Type::Emoji);
            }
        }

        // Commands
        for (auto &command : getApp()->commands->items.getVector()) {
            addString(command.name, TaggedString::Command);
        }

        for (auto &command :
             getApp()->commands->getDefaultTwitchCommandList()) {
            addString(command, TaggedString::Command);
        }
    }
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
        auto p = this->emotes_.insert(ts);
        if (!p.second) {
            // No inseration was made, figure out if we need to replace the
            // username.

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
