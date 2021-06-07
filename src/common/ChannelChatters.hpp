#pragma once

#include "common/Channel.hpp"
#include "common/ChatterSet.hpp"
#include "common/UniqueAccess.hpp"
#include "lrucache/lrucache.hpp"
#include "util/QStringHash.hpp"

#include <QRgb>

namespace chatterino {

class ChannelChatters
{
public:
    ChannelChatters(Channel &channel);
    virtual ~ChannelChatters() = default;  // add vtable

    SharedAccessGuard<const ChatterSet> accessChatters() const;

    void addRecentChatter(const QString &user);
    void addJoinedUser(const QString &user);
    void addPartedUser(const QString &user);
    const QColor getUserColor(const QString &user);
    void setUserColor(const QString &user, const QColor &color);
    void updateOnlineChatters(const std::unordered_set<QString> &chatters);

private:
    static constexpr int maxChatterColorCount = 5000;

    Channel &channel_;

    // maps 2 char prefix to set of names
    UniqueAccess<ChatterSet> chatters_;
    UniqueAccess<cache::lru_cache<QString, QRgb>> chatterColors_;

    // combines multiple joins/parts into one message
    UniqueAccess<QStringList> joinedUsers_;
    bool joinedUsersMergeQueued_ = false;
    UniqueAccess<QStringList> partedUsers_;
    bool partedUsersMergeQueued_ = false;

    QObject lifetimeGuard_;
};

}  // namespace chatterino
