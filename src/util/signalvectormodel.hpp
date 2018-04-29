#pragma once

#include <QAbstractTableModel>
#include <QStandardItem>
#include <util/signalvector2.hpp>

#include <pajlada/signals/signalholder.hpp>

namespace chatterino {
namespace util {

template <typename TVectorItem>
class SignalVectorModel : public QAbstractTableModel, pajlada::Signals::SignalHolder
{
public:
    SignalVectorModel(util::BaseSignalVector<TVectorItem> *vec, int columnCount,
                      QObject *parent = nullptr)
        : QAbstractTableModel(parent)
        , _columnCount(columnCount)
    {
        this->managedConnect(vec->itemInserted, [this](auto args) {
            std::vector<QStandardItem *> items;
            for (int i = 0; i < this->_columnCount; i++) {
                items.push_back(new QStandardItem());
            }

            int row = this->prepareInsert(args.item, args.index, items);
            assert(row >= 0 && row <= this->rows.size());

            // insert row
            this->beginInsertRows(QModelIndex(), row, row);
            this->rows.insert(this->rows.begin() + row, Row(items));
            this->endInsertRows();
        });
        this->managedConnect(vec->itemRemoved, [this](auto args) {
            int row = this->prepareRemove(args.item, args.index);
            assert(row >= 0 && row <= this->rows.size());

            // remove row
            this->beginRemoveRows(QModelIndex(), row, row);
            for (QStandardItem *item : this->rows[row].items) {
                delete item;
            }
            this->rows.erase(this->rows.begin() + row);
            this->endRemoveRows();
        });
    }

    virtual ~SignalVectorModel()
    {
        for (Row &row : this->rows) {
            for (QStandardItem *item : row.items) {
                delete item;
            }
        }
    }

    int rowCount(const QModelIndex &parent) const
    {
        return this->rows.size();
    }

    int columnCount(const QModelIndex &parent) const
    {
        return this->_columnCount;
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        int row = index.row(), column = index.column();
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        return rows[row].items[column]->data(role);
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        this->rows[index.row()].items[index.column()]->setData(value, role);

        return true;
    }

    QStandardItem *getItem(int row, int column)
    {
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        return rows[row][column];
    }

protected:
    virtual int prepareInsert(const TVectorItem &item, int index,
                              std::vector<QStandardItem *> &rowToAdd) = 0;
    virtual int prepareRemove(const TVectorItem &item, int index) = 0;

private:
    struct Row {
        std::vector<QStandardItem *> items;
        bool isCustomRow;

        Row(const std::vector<QStandardItem *> _items, bool _isCustomRow = false)
            : items(_items)
            , isCustomRow(_isCustomRow)
        {
        }
    };

    std::vector<Row> rows;
    int _columnCount;
};

}  // namespace util
}  // namespace chatterino
