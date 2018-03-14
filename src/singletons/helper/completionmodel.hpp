#pragma once

#include <QAbstractListModel>
#include <set>

#include <map>
#include <string>

#include "common.hpp"

namespace chatterino {
namespace singletons {
class CompletionModel : public QAbstractListModel
{
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
    void addString(const std::string &str);
    void addString(const QString &str);

    void addUser(const QString &str);

private:
    struct TaggedString {
        QString str;
        // emote == true
        // username == false
        bool isEmote = true;
        bool operator<(const TaggedString& that) const {
            if (this->isEmote) {
                if (that.isEmote) {
                    return this->str < that.str;
                } else return true;
            } else {
                if (that.isEmote) return false;
                else return this->str < that.str;
            }
        }

    };
    TaggedString createEmote(const std::string &str) {
        return TaggedString{qS(str), true};
    }
    TaggedString createEmote(const QString &str) {
        return TaggedString{str, true};
    }
    TaggedString createUser(const QString &str) {
        return TaggedString{str, false};
    }
    std::set<TaggedString> emotes;

    QString channelName;
};
}
}
