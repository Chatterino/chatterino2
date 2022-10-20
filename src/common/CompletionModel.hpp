#pragma once

#include <QAbstractListModel>

#include <chrono>
#include <mutex>
#include <set>

namespace chatterino {

class Channel;

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
            SeventvGlobalEmote,
            SeventvChannelEmote,
            TwitchGlobalEmote,
            TwitchLocalEmote,
            TwitchSubscriberEmote,
            Emoji,
            EmoteEnd,
            // end emotes

            CustomCommand,
            ChatterinoCommand,
            TwitchCommand,
        };

        TaggedString(const QString &string, Type type);

        bool isEmote() const;
        bool operator<(const TaggedString &that) const;

        QString string;
        Type type;
    };

public:
    CompletionModel(Channel &channel);

    virtual int columnCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex &index, int) const override;
    virtual int rowCount(const QModelIndex &) const override;

    void refresh(const QString &prefix, bool isFirstWord = false);

    static bool compareStrings(const QString &a, const QString &b);

private:
    std::set<TaggedString> items_;
    mutable std::mutex itemsMutex_;
    Channel &channel_;
};

}  // namespace chatterino
