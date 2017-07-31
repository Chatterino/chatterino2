#pragma once

#include <QAbstractListModel>
#include <QVector>

#include <string>

namespace chatterino {

class EmoteManager;

class CompletionModel : public QAbstractListModel
{
public:
    virtual int columnCount(const QModelIndex & /*parent*/) const override
    {
        return 1;
    }

    virtual QVariant data(const QModelIndex &index, int role) const override
    {
        // TODO: Implement more safely
        return QVariant(this->emotes.at(index.row()));
    }

    virtual int rowCount(const QModelIndex &parent) const override
    {
        return this->emotes.size();
    }

    void addString(const std::string &str);

    QVector<QString> emotes;
};

class CompletionManager
{
    CompletionManager(EmoteManager &_emoteManager);

    EmoteManager &emoteManager;

public:
    CompletionModel *createModel(const std::string &channelName);
    void updateModel(CompletionModel *model, const std::string &channelName = std::string());

    friend class Application;
};

}  // namespace chatterino
