#pragma once

#include <QAbstractListModel>
#include <QVector>

#include <map>
#include <string>

namespace chatterino {

class CompletionModel : public QAbstractListModel
{
public:
    CompletionModel(const QString &_channelName);

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

    void refresh();

private:
    void addString(const std::string &str);
    void addString(const QString &str);

    QVector<QString> emotes;

    QString channelName;
};

class CompletionManager
{
    CompletionManager() = default;

    std::map<std::string, CompletionModel *> models;

public:
    static CompletionManager &getInstance()
    {
        static CompletionManager instance;
        return instance;
    }

    CompletionModel *createModel(const std::string &channelName);
};

}  // namespace chatterino
