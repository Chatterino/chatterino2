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
    SignalVectorModel(int columnCount, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
        , _columnCount(columnCount)
    {
        for (int i = 0; i < columnCount; i++) {
            this->_headerData.emplace_back();
        }
    }

    void init(util::BaseSignalVector<TVectorItem> *vec)
    {
        this->vector = vec;

        auto insert = [this](const typename BaseSignalVector<TVectorItem>::ItemArgs &args) {
            std::vector<QStandardItem *> items;
            for (int i = 0; i < this->_columnCount; i++) {
                items.push_back(new QStandardItem());
            }

            int row = this->prepareVectorInserted(args.item, args.index, items);
            assert(row >= 0 && row <= this->rows.size());

            // insert row
            this->beginInsertRows(QModelIndex(), row, row);
            this->rows.insert(this->rows.begin() + row, Row(items));
            this->endInsertRows();
        };

        int i = 0;
        for (const TVectorItem &item : vec->getVector()) {
            typename BaseSignalVector<TVectorItem>::ItemArgs args{item, i++, 0};

            insert(args);
        }

        this->managedConnect(vec->itemInserted, insert);

        this->managedConnect(vec->itemRemoved, [this](auto args) {
            int row = this->prepareVectorRemoved(args.item, args.index);
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

    virtual int rowCount(const QModelIndex &parent) const
    {
        return this->rows.size();
    }

    virtual int columnCount(const QModelIndex &parent) const
    {
        return this->_columnCount;
    }

    virtual QVariant data(const QModelIndex &index, int role) const
    {
        int row = index.row(), column = index.column();
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        return rows[row].items[column]->data(role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        this->rows[index.row()].items[index.column()]->setData(value, role);

        return true;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal) {
            return QVariant();
        }

        auto it = this->_headerData[section].find(role);
        if (it == this->_headerData[section].end()) {
            return QVariant();
        } else {
            return it.value();
        }
    }

    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                       int role = Qt::DisplayRole)
    {
        if (orientation != Qt::Horizontal) {
            return false;
        }

        this->_headerData[section][role] = value;
        return true;
    }

    virtual QStandardItem *getItem(int row, int column)
    {
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        return rows[row].items[column];
    }

    void removeRow(int row)
    {
        assert(row >= 0 && row <= this->rows.size());

        int signalVectorRow = this->prepareModelItemRemoved(row);
        this->vector->removeItem(signalVectorRow);
    }

protected:
    // gets called when an item gets inserted into the SignalVector
    //
    // returns the index of that the row should be inserted into and edits the rowToAdd elements
    // based on the item
    virtual int prepareVectorInserted(const TVectorItem &item, int index,
                                      std::vector<QStandardItem *> &rowToAdd) = 0;
    // gets called when an item gets removed from a SignalVector
    //
    // returns the index of the row in the model that should be removed
    virtual int prepareVectorRemoved(const TVectorItem &item, int index) = 0;

    // gets called when an item gets removed from the model
    //
    // returns the related index of the SignalVector
    virtual int prepareModelItemRemoved(int index) = 0;

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
    std::vector<QMap<int, QVariant>> _headerData;
    BaseSignalVector<TVectorItem> *vector;

    int _columnCount;
};

}  // namespace util
}  // namespace chatterino
