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
            if (args.caller == this) {
                return;
            }

            // get row index
            int row = this->getModelIndexFromVectorIndex(args.index);
            assert(row >= 0 && row <= this->rows.size());

            // get row items
            std::vector<QStandardItem *> items;
            for (int i = 0; i < this->_columnCount; i++) {
                items.push_back(new QStandardItem());
            }

            this->getRowFromItem(args.item, items);

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
            if (args.caller == this) {
                return;
            }

            int row = this->getModelIndexFromVectorIndex(args.index);
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
        int row = index.row(), column = index.column();
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        this->rows[row].items[column]->setData(value, role);

        int vecRow = this->getVectorIndexFromModelIndex(row);
        this->vector->removeItem(vecRow, this);
        TVectorItem item = this->getItemFromRow(this->rows[row].items);
        this->vector->insertItem(item, vecRow, this);

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

    Qt::ItemFlags flags(const QModelIndex &index) const
    {
        int row = index.row(), column = index.column();
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        return this->rows[index.row()].items[index.column()]->flags();
    }

    virtual QStandardItem *getItem(int row, int column)
    {
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        return rows[row].items[column];
    }

    void removeRow(int row)
    {
        assert(row >= 0 && row <= this->rows.size());

        int signalVectorRow = this->getVectorIndexFromModelIndex(row);
        this->vector->removeItem(signalVectorRow);
    }

protected:
    // turn a vector item into a model row
    virtual TVectorItem getItemFromRow(std::vector<QStandardItem *> &row) = 0;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const TVectorItem &item, std::vector<QStandardItem *> &row) = 0;

    // returns the related index of the SignalVector
    virtual int getVectorIndexFromModelIndex(int index) = 0;

    // returns the related index of the model
    virtual int getModelIndexFromVectorIndex(int index) = 0;

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
