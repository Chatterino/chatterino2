#pragma once

#include <QAbstractListModel>

#include <chrono>
#include <mutex>
#include <set>

namespace chatterino {

class TwitchChannel;

class CompletionModel : public QAbstractListModel
{
    struct TaggedString {
        enum Type {
            Username,

            // emotes
            EmoteStart,
            FFZGlobalEmote,
            FFZChannelEmote,
            BTTVGlobalEmote,
            BTTVChannelEmote,
            TwitchGlobalEmote,
            TwitchSubscriberEmote,
            Emoji,
            EmoteEnd,
            // end emotes

            Command,
        };

        TaggedString(const QString &_str, Type _type);

        bool isExpired(const std::chrono::steady_clock::time_point &now) const;
        bool isEmote() const;
        bool operator<(const TaggedString &that) const;

        QString str;
        // Type will help decide the lifetime of the tagged strings
        Type type;

        mutable std::chrono::steady_clock::time_point timeAdded;
    };

public:
    CompletionModel(const QString &channelName);

    virtual int columnCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex &index, int) const override;
    virtual int rowCount(const QModelIndex &) const override;

    void refresh();
    void addString(const QString &str, TaggedString::Type type);
    void addUser(const QString &str);

    void clearExpiredStrings();

private:
    TaggedString createUser(const QString &str);

    mutable std::mutex emotesMutex_;
    std::set<TaggedString> emotes_;

    QString channelName_;
};

}  // namespace chatterino
