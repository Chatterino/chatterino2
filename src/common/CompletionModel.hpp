#pragma once

#include <QAbstractListModel>

#include <chrono>
#include <mutex>
#include <set>
#include <shared_mutex>

class InputCompletionTest;

namespace chatterino {

class Channel;

class CompletionModel : public QAbstractListModel
{
public:
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
#ifdef CHATTERINO_HAVE_PLUGINS
            PluginCommand,
            CustomCompletion,
#endif
        };

        TaggedString(QString _string, Type type);

        bool isEmote() const;
        bool operator<(const TaggedString &that) const;

        const QString string;
        const Type type;
    };

    CompletionModel(Channel &channel);

    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent) const override;

    void refresh(const QString &text, const QString &prefix,
                 bool isFirstWord = false);

    static bool compareStrings(const QString &a, const QString &b);

private:
    std::set<TaggedString> items_;
    mutable std::shared_mutex itemsMutex_;

    Channel &channel_;

    void addItems(const QString &text, const QString &prefix, bool isFirstWord);
    friend class ::InputCompletionTest;
};

}  // namespace chatterino
