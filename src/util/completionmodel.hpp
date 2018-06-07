#pragma once

#include "common.hpp"

#include <QAbstractListModel>

#include <chrono>
#include <mutex>
#include <set>

namespace chatterino {

class CompletionModel : public QAbstractListModel
{
    struct TaggedString {
        enum Type {
            Username,

            // Emotes
            FFZGlobalEmote = 20,
            FFZChannelEmote,
            BTTVGlobalEmote,
            BTTVChannelEmote,
            TwitchGlobalEmote,
            TwitchSubscriberEmote,
            Emoji,
        };

        TaggedString(const QString &_str, Type _type)
            : str(_str)
            , type(_type)
            , timeAdded(std::chrono::steady_clock::now())
        {
        }

        QString str;

        // Type will help decide the lifetime of the tagged strings
        Type type;

        mutable std::chrono::steady_clock::time_point timeAdded;

        bool HasExpired(const std::chrono::steady_clock::time_point &now) const
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

        bool IsEmote() const
        {
            return this->type >= 20;
        }

        bool operator<(const TaggedString &that) const
        {
            if (this->IsEmote()) {
                if (that.IsEmote()) {
                    int k = QString::compare(this->str, that.str, Qt::CaseInsensitive);
                    if (k == 0) {
                        return this->str > that.str;
                    }

                    return k < 0;
                }

                return true;
            }

            if (that.IsEmote()) {
                return false;
            }

            int k = QString::compare(this->str, that.str, Qt::CaseInsensitive);
            if (k == 0) {
                return false;
            }

            return k < 0;
        }
    };

public:
    CompletionModel(const QString &_channelName);

    int columnCount(const QModelIndex &) const override
    {
        return 1;
    }

    QVariant data(const QModelIndex &index, int) const override
    {
        std::lock_guard<std::mutex> lock(this->emotesMutex);

        // TODO: Implement more safely
        auto it = this->emotes.begin();
        std::advance(it, index.row());
        return QVariant(it->str);
    }

    int rowCount(const QModelIndex &) const override
    {
        std::lock_guard<std::mutex> lock(this->emotesMutex);

        return this->emotes.size();
    }

    void refresh();
    void addString(const QString &str, TaggedString::Type type);

    void addUser(const QString &str);

    void ClearExpiredStrings();

private:
    TaggedString createUser(const QString &str)
    {
        return TaggedString{str, TaggedString::Type::Username};
    }

    mutable std::mutex emotesMutex;
    std::set<TaggedString> emotes;

    QString channelName;
};

}  // namespace chatterino
