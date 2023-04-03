#include "common/CompletionModel.hpp"

#include "Application.hpp"
#include "common/ChatterSet.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "messages/Emote.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/QStringHash.hpp"

#include <QtAlgorithms>

#include <utility>

namespace chatterino {

//
// TaggedString
//

CompletionModel::TaggedString::TaggedString(QString _string, Type _type)
    : string(std::move(_string))
    , type(_type)
{
}

bool CompletionModel::TaggedString::isEmote() const
{
    return this->type > Type::EmoteStart && this->type < Type::EmoteEnd;
}

bool CompletionModel::TaggedString::operator<(const TaggedString &that) const
{
    if (this->isEmote() != that.isEmote())
    {
        return this->isEmote();
    }

    return CompletionModel::compareStrings(this->string, that.string);
}

//
// CompletionModel
//
CompletionModel::CompletionModel(Channel &channel)
    : channel_(channel)
{
}

int CompletionModel::columnCount(const QModelIndex &parent) const
{
    (void)parent;  // unused

    return 1;
}

QVariant CompletionModel::data(const QModelIndex &index, int role) const
{
    (void)role;  // unused

    std::shared_lock lock(this->itemsMutex_);

    auto it = this->items_.begin();
    std::advance(it, index.row());
    return {it->string};
}

int CompletionModel::rowCount(const QModelIndex &parent) const
{
    (void)parent;  // unused

    std::shared_lock lock(this->itemsMutex_);

    return this->items_.size();
}

void CompletionModel::refresh(const QString &prefix, bool isFirstWord)
{
    std::unique_lock lock(this->itemsMutex_);

    this->items_.clear();

    if (prefix.length() < 2 || !this->channel_.isTwitchChannel())
    {
        return;
    }

    // Twitch channel
    auto *tc = dynamic_cast<TwitchChannel *>(&this->channel_);

    auto addString = [=, this](const QString &str, TaggedString::Type type) {
        // Special case for handling default Twitch commands
        if (type == TaggedString::TwitchCommand)
        {
            if (prefix.size() < 2)
            {
                return;
            }

            auto prefixChar = prefix.at(0);

            static std::set<QChar> validPrefixChars{'/', '.'};

            if (validPrefixChars.find(prefixChar) == validPrefixChars.end())
            {
                return;
            }

            if (startsWithOrContains((prefixChar + str), prefix,
                                     Qt::CaseInsensitive,
                                     getSettings()->prefixOnlyEmoteCompletion))
            {
                this->items_.emplace((prefixChar + str + " "), type);
            }

            return;
        }

        if (startsWithOrContains(str, prefix, Qt::CaseInsensitive,
                                 getSettings()->prefixOnlyEmoteCompletion))
        {
            this->items_.emplace(str + " ", type);
        }
    };

    if (auto account = getApp()->accounts->twitch.getCurrent())
    {
        // Twitch Emotes available globally
        for (const auto &emote : account->accessEmotes()->emotes)
        {
            addString(emote.first.string, TaggedString::TwitchGlobalEmote);
        }

        // Twitch Emotes available locally
        auto localEmoteData = account->accessLocalEmotes();
        if (tc != nullptr &&
            localEmoteData->find(tc->roomId()) != localEmoteData->end())
        {
            for (const auto &emote : localEmoteData->at(tc->roomId()))
            {
                addString(emote.first.string,
                          TaggedString::Type::TwitchLocalEmote);
            }
        }
    }

    // 7TV Global
    for (const auto &emote :
         *getApp()->twitch->getSeventvEmotes().globalEmotes())
    {
        addString(emote.first.string, TaggedString::Type::SeventvGlobalEmote);
    }

    // Bttv Global
    for (const auto &emote : *getApp()->twitch->getBttvEmotes().emotes())
    {
        addString(emote.first.string, TaggedString::Type::BTTVChannelEmote);
    }

    // Ffz Global
    for (const auto &emote : *getApp()->twitch->getFfzEmotes().emotes())
    {
        addString(emote.first.string, TaggedString::Type::FFZChannelEmote);
    }

    // Emojis
    if (prefix.startsWith(":"))
    {
        const auto &emojiShortCodes = getApp()->emotes->emojis.shortCodes;
        for (const auto &m : emojiShortCodes)
        {
            addString(QString(":%1:").arg(m), TaggedString::Type::Emoji);
        }
    }

    //
    // Stuff below is available only in regular Twitch channels
    if (tc == nullptr)
    {
        return;
    }

    // Usernames
    if (prefix.startsWith("@"))
    {
        QString usernamePrefix = prefix;
        usernamePrefix.remove(0, 1);

        auto chatters = tc->accessChatters()->filterByPrefix(usernamePrefix);

        for (const auto &name : chatters)
        {
            addString(
                "@" + formatUserMention(name, isFirstWord,
                                        getSettings()->mentionUsersWithComma,
                                        getSettings()->lowercaseUsernames),
                TaggedString::Type::Username);
        }
    }
    else if (!getSettings()->userCompletionOnlyWithAt)
    {
        auto chatters = tc->accessChatters()->filterByPrefix(prefix);

        for (const auto &name : chatters)
        {
            addString(formatUserMention(name, isFirstWord,
                                        getSettings()->mentionUsersWithComma,
                                        getSettings()->lowercaseUsernames),
                      TaggedString::Type::Username);
        }
    }

    // 7TV Channel
    for (const auto &emote : *tc->seventvEmotes())
    {
        addString(emote.first.string, TaggedString::Type::SeventvChannelEmote);
    }

    // Bttv Channel
    for (const auto &emote : *tc->bttvEmotes())
    {
        addString(emote.first.string, TaggedString::Type::BTTVGlobalEmote);
    }

    // Ffz Channel
    for (const auto &emote : *tc->ffzEmotes())
    {
        addString(emote.first.string, TaggedString::Type::BTTVGlobalEmote);
    }
#ifdef CHATTERINO_HAVE_PLUGINS
    for (const auto &command : getApp()->commands->pluginCommands())
    {
        addString(command, TaggedString::PluginCommand);
    }
#endif
    // Custom Chatterino commands
    for (const auto &command : getApp()->commands->items)
    {
        addString(command.name, TaggedString::CustomCommand);
    }

    // Default Chatterino commands
    for (const auto &command :
         getApp()->commands->getDefaultChatterinoCommandList())
    {
        addString(command, TaggedString::ChatterinoCommand);
    }

    // Default Twitch commands
    for (const auto &command : TWITCH_DEFAULT_COMMANDS)
    {
        addString(command, TaggedString::TwitchCommand);
    }
}

bool CompletionModel::compareStrings(const QString &a, const QString &b)
{
    // try comparing insensitively, if they are the same then senstively
    // (fixes order of LuL and LUL)
    int k = QString::compare(a, b, Qt::CaseInsensitive);
    if (k == 0)
    {
        return a > b;
    }

    return k < 0;
}

}  // namespace chatterino
