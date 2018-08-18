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

//
// TaggedString
//

CompletionModel::TaggedString::TaggedString(const QString &_string, Type _type)
    : string(_string)
    , type(_type)
{
}

bool CompletionModel::TaggedString::isEmote() const
{
    return this->type > Type::EmoteStart && this->type < Type::EmoteEnd;
}

bool CompletionModel::TaggedString::operator<(const TaggedString &that) const
{
    if (this->isEmote() != that.isEmote()) {
        return this->isEmote();
    }

    // try comparing insensitively, if they are the same then senstively
    // (fixes order of LuL and LUL)
    int k = QString::compare(this->string, that.string, Qt::CaseInsensitive);
    if (k == 0) return this->string > that.string;

    return k < 0;
}

//
// CompletionModel
//
CompletionModel::CompletionModel(Channel &channel)
    : channel_(channel)
{
}

int CompletionModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant CompletionModel::data(const QModelIndex &index, int) const
{
    std::lock_guard<std::mutex> lock(this->itemsMutex_);

    auto it = this->items_.begin();
    std::advance(it, index.row());
    return QVariant(it->string);
}

int CompletionModel::rowCount(const QModelIndex &) const
{
    std::lock_guard<std::mutex> lock(this->itemsMutex_);

    return this->items_.size();
}

void CompletionModel::refresh(const QString &prefix)
{
    std::lock_guard<std::mutex> guard(this->itemsMutex_);
    this->items_.clear();

    if (prefix.length() < 2) return;

    auto addString = [&](const QString &str, TaggedString::Type type) {
        if (str.startsWith(prefix, Qt::CaseInsensitive))
            this->items_.emplace(str + " ", type);
    };

    if (auto channel = dynamic_cast<TwitchChannel *>(&this->channel_)) {
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

}  // namespace chatterino
