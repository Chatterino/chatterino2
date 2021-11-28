#include "ChannelChatters.hpp"

#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"

namespace chatterino {

ChannelChatters::ChannelChatters(Channel &channel)
    : channel_(channel)
    , chatterColors_(ChannelChatters::maxChatterColorCount)
{
}

SharedAccessGuard<const ChatterSet> ChannelChatters::accessChatters() const
{
    return this->chatters_.accessConst();
}

void ChannelChatters::addRecentChatter(const QString &user)
{
    auto chatters = this->chatters_.access();
    chatters->addRecentChatter(user);
}

void ChannelChatters::addJoinedUser(const QString &user)
{
    auto joinedUsers = this->joinedUsers_.access();
    joinedUsers->append(user);

    if (!this->joinedUsersMergeQueued_)
    {
        this->joinedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->lifetimeGuard_, [this] {
            auto joinedUsers = this->joinedUsers_.access();

            MessageBuilder builder;
            TwitchMessageBuilder::listOfUsersSystemMessage(
                "Users joined:", *joinedUsers, &this->channel_, &builder);
            builder->flags.set(MessageFlag::Collapsed);
            joinedUsers->clear();
            this->channel_.addMessage(builder.release());
            this->joinedUsersMergeQueued_ = false;
        });
    }
}

void ChannelChatters::addPartedUser(const QString &user)
{
    auto partedUsers = this->partedUsers_.access();
    partedUsers->append(user);

    if (!this->partedUsersMergeQueued_)
    {
        this->partedUsersMergeQueued_ = true;

        QTimer::singleShot(500, &this->lifetimeGuard_, [this] {
            auto partedUsers = this->partedUsers_.access();

            MessageBuilder builder;
            TwitchMessageBuilder::listOfUsersSystemMessage(
                "Users parted:", *partedUsers, &this->channel_, &builder);
            builder->flags.set(MessageFlag::Collapsed);
            this->channel_.addMessage(builder.release());
            partedUsers->clear();

            this->partedUsersMergeQueued_ = false;
        });
    }
}

void ChannelChatters::updateOnlineChatters(
    const std::unordered_set<QString> &usernames)
{
    auto chatters = this->chatters_.access();
    chatters->updateOnlineChatters(usernames);
}

size_t ChannelChatters::colorsSize() const
{
    auto size = this->chatterColors_.access()->size();
    return size;
}

const QColor ChannelChatters::getUserColor(const QString &user)
{
    const auto chatterColors = this->chatterColors_.access();

    auto lowerUser = user.toLower();

    if (!chatterColors->exists(lowerUser))
    {
        // Returns an invalid color so we can decide not to override `textColor`
        return QColor();
    }

    return QColor::fromRgb(chatterColors->get(lowerUser));
}

void ChannelChatters::setUserColor(const QString &user, const QColor &color)
{
    const auto chatterColors = this->chatterColors_.access();
    chatterColors->put(user.toLower(), color.rgb());
}

}  // namespace chatterino
