#include "ChannelChatters.hpp"

#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"

namespace chatterino {

ChannelChatters::ChannelChatters(Channel &channel)
    : channel_(channel)
{
}

AccessGuard<const UsernameSet> ChannelChatters::accessChatters() const
{
    return this->chatters_.accessConst();
}

void ChannelChatters::addRecentChatter(const QString &user)
{
    this->chatters_.access()->insert(user);
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

            MessageBuilder builder(systemMessage,
                                   "Users joined: " + joinedUsers->join(", "));
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

            MessageBuilder builder(systemMessage,
                                   "Users parted: " + partedUsers->join(", "));
            builder->flags.set(MessageFlag::Collapsed);
            this->channel_.addMessage(builder.release());
            partedUsers->clear();

            this->partedUsersMergeQueued_ = false;
        });
    }
}

void ChannelChatters::setChatters(UsernameSet &&set)
{
    this->chatters_.access()->merge(std::move(set));
}

const QColor ChannelChatters::getUserColor(const QString &user)
{
    const auto chattersColors = this->chattersColors_.accessConst();

    const auto search = chattersColors->find(user.toLower());
    if (search == chattersColors->end())
    {
        // Returns an invalid color so we can decide not to override `textColor`
        return QColor();
    }

    return search->second;
}

void ChannelChatters::setUserColor(const QString &user, const QColor &color)
{
    const auto chattersColors = this->chattersColors_.access();
    chattersColors->insert_or_assign(user, color);
}

void ChannelChatters::removeUserColor(const QString &user)
{
    const auto chattersColors = this->chattersColors_.access();
    chattersColors->erase(user);
}

}  // namespace chatterino
