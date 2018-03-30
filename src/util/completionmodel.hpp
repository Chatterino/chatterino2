#pragma once

#include "common.hpp"

#include <QAbstractListModel>

#include <set>
#include <string>

namespace chatterino {

class CompletionModel : public QAbstractListModel
{
    struct TaggedString {
        QString str;

        // Type will help decide the lifetime of the tagged strings
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
        } type;

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
                    } else {
                        return k < 0;
                    }
                } else {
                    return true;
                }
            } else {
                if (that.IsEmote()) {
                    return false;
                } else {
                    int k = QString::compare(this->str, that.str, Qt::CaseInsensitive);
                    if (k == 0) {
                        return false;
                    } else {
                        return k < 0;
                    }
                }
            }
        }
    };

public:
    CompletionModel(const QString &_channelName);

    virtual int columnCount(const QModelIndex &) const override
    {
        return 1;
    }

    virtual QVariant data(const QModelIndex &index, int) const override
    {
        // TODO: Implement more safely
        auto it = this->emotes.begin();
        std::advance(it, index.row());
        return QVariant(it->str);
    }

    virtual int rowCount(const QModelIndex &) const override
    {
        return this->emotes.size();
    }

    void refresh();
    void addString(const std::string &str, TaggedString::Type type);
    void addString(const QString &str, TaggedString::Type type);

    void addUser(const QString &str);

private:
    TaggedString createUser(const QString &str)
    {
        return TaggedString{str, TaggedString::Type::Username};
    }

    std::set<TaggedString> emotes;

    QString channelName;
};

}  // namespace chatterino
