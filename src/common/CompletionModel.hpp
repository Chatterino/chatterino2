#pragma once

#include <QAbstractListModel>

#include <chrono>
#include <set>
#include <shared_mutex>

class InputCompletionTest;

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
            SeventvPersonalEmote,
            TwitchGlobalEmote,
            TwitchLocalEmote,
            TwitchSubscriberEmote,
            Emoji,
            EmoteEnd,
            // end emotes

            CustomCommand,
            ChatterinoCommand,
            TwitchCommand,
#ifdef CHATTERINO_HAVE_PLUGINS
            PluginCommand,
#endif
        };

        TaggedString(QString _string, Type type);

        bool isEmote() const;
        bool operator<(const TaggedString &that) const;

        const QString string;
        const Type type;
    };

public:
    CompletionModel(Channel &channel);

    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;

    void refresh(const QString &prefix, bool isFirstWord = false);

    static bool compareStrings(const QString &a, const QString &b);

private:
    std::vector<QString> allItems() const;

    mutable std::shared_mutex itemsMutex_;
    std::set<TaggedString> items_;

    Channel &channel_;

    friend class ::InputCompletionTest;
};

}  // namespace chatterino
