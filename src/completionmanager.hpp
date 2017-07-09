#pragma once

#include <QAbstractItemModel>

#include <string>

namespace chatterino {

class CompletionModel : public QAbstractItemModel
{
public:
    virtual int columnCount(const QModelIndex & /*parent*/) const override
    {
        return 1;
    }

    virtual QVariant data(const QModelIndex &index, int role) const override
    {
        // TODO: Implement
        return QVariant();
    }

    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        // TODO: Implement
        return QModelIndex();
    }

    virtual QModelIndex parent(const QModelIndex &child) const override
    {
        return QModelIndex();
    }

    virtual int rowCount(const QModelIndex &parent) const override
    {
        return 1;
    }
};

class CompletionManager
{
    CompletionManager();

public:
    CompletionModel *createModel(const std::string &channelName);
    void updateModel(CompletionModel *model, const std::string &channelName);

    friend class Application;
};

}  // namespace chatterino
