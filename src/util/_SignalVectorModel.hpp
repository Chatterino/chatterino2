#pragma once

#include <QAbstractTableModel>
#include <QStandardItem>
#include <boost/optional.hpp>
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

        auto insert = [this](const SignalVectorItemArgs<TVectorItem> &args) {
            if (args.caller == this) {
                return;
            }
            // get row index
            int index = this->getModelIndexFromVectorIndex(args.index);
            assert(index >= 0 && index <= this->rows.size());

            // get row items
            std::vector<QStandardItem *> row = this->createRow();
            this->getRowFromItem(args.item, row);

            // insert row
            index = this->beforeInsert(args.item, row, index);

            this->beginInsertRows(QModelIndex(), index, index);
            this->rows.insert(this->rows.begin() + index, Row(row, args.item));
            this->endInsertRows();
        };

        int i = 0;
        for (const TVectorItem &item : vec->getVector()) {
            SignalVectorItemArgs<TVectorItem> args{item, i++, 0};

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
            std::vector<QStandardItem *> items = std::move(this->rows[row].items);

            this->beginRemoveRows(QModelIndex(), row, row);
            this->rows.erase(this->rows.begin() + row);
            this->endRemoveRows();

            this->afterRemoved(args.item, items, row);

            for (QStandardItem *item : items) {
                delete item;
            }
        });

        this->afterInit();
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

        Row &rowItem = this->rows[row];

        rowItem.items[column]->setData(value, role);

        if (rowItem.isCustomRow) {
            this->customRowSetData(rowItem.items, column, value, role);
        } else {
            int vecRow = this->getVectorIndexFromModelIndex(row);
            this->vector->removeItem(vecRow, this);

            assert(this->rows[row].original);
            TVectorItem item =
                this->getItemFromRow(this->rows[row].items, this->rows[row].original.get());
            this->vector->insertItem(item, vecRow, this);
        }

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

        emit this->headerDataChanged(Qt::Horizontal, section, section);
        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const
    {
        int row = index.row(), column = index.column();
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        return this->rows[index.row()].items[index.column()]->flags();
    }

    QStandardItem *getItem(int row, int column)
    {
        assert(row >= 0 && row < this->rows.size() && column >= 0 && column < this->_columnCount);

        return rows[row].items[column];
    }

    void deleteRow(int row)
    {
        int signalVectorRow = this->getVectorIndexFromModelIndex(row);
        this->vector->removeItem(signalVectorRow);
    }

    virtual bool removeRows(int row, int count, const QModelIndex &parent) override
    {
        if (count != 1) {
            return false;
        }

        assert(row >= 0 && row < this->rows.size());

        int signalVectorRow = this->getVectorIndexFromModelIndex(row);
        this->vector->removeItem(signalVectorRow);

        return true;
    }

protected:
    virtual void afterInit()
    {
    }

    // turn a vector item into a model row
    virtual TVectorItem getItemFromRow(std::vector<QStandardItem *> &row,
                                       const TVectorItem &original) = 0;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const TVectorItem &item, std::vector<QStandardItem *> &row) = 0;

    virtual int beforeInsert(const TVectorItem &item, std::vector<QStandardItem *> &row,
                             int proposedIndex)
    {
        return proposedIndex;
    }

    virtual void afterRemoved(const TVectorItem &item, std::vector<QStandardItem *> &row, int index)
    {
    }

    virtual void customRowSetData(const std::vector<QStandardItem *> &row, int column,
                                  const QVariant &value, int role)
    {
    }

    void insertCustomRow(std::vector<QStandardItem *> row, int index)
    {
        assert(index >= 0 && index <= this->rows.size());

        this->beginInsertRows(QModelIndex(), index, index);
        this->rows.insert(this->rows.begin() + index, Row(std::move(row), true));
        this->endInsertRows();
    }

    void removeCustomRow(int index)
    {
        assert(index >= 0 && index <= this->rows.size());
        assert(this->rows[index].isCustomRow);

        this->beginRemoveRows(QModelIndex(), index, index);
        this->rows.erase(this->rows.begin() + index);
        this->endRemoveRows();
    }

    std::vector<QStandardItem *> createRow()
    {
        std::vector<QStandardItem *> row;
        for (int i = 0; i < this->_columnCount; i++) {
            row.push_back(new QStandardItem());
        }
        return row;
    }

    struct Row {
        std::vector<QStandardItem *> items;
        boost::optional<TVectorItem> original;
        bool isCustomRow;

        Row(std::vector<QStandardItem *> _items, bool _isCustomRow = false)
            : items(std::move(_items))
            , isCustomRow(_isCustomRow)
        {
        }

        Row(std::vector<QStandardItem *> _items, const TVectorItem &_original,
            bool _isCustomRow = false)
            : items(std::move(_items))
            , original(_original)
            , isCustomRow(_isCustomRow)
        {
        }
    };
    std::vector<Row> rows;

private:
    std::vector<QMap<int, QVariant>> _headerData;
    BaseSignalVector<TVectorItem> *vector;

    int _columnCount;

    // returns the related index of the SignalVector
    int getVectorIndexFromModelIndex(int index)
    {
        int i = 0;

        for (auto &row : this->rows) {
            if (row.isCustomRow) {
                index--;
                continue;
            }

            if (i == index) {
                return i;
            }
            i++;
        }

        return i;
    }

    // returns the related index of the model
    int getModelIndexFromVectorIndex(int index)
    {
        int i = 0;

        for (auto &row : this->rows) {
            if (row.isCustomRow) {
                index++;
            }

            if (i == index) {
                return i;
            }
            i++;
        }

        return i;
    }
};

}  // namespace util
}  // namespace chatterino
