#pragma once

#include "common/SignalVector.hpp"

#include <QAbstractTableModel>
#include <QStandardItem>
#include <boost/optional.hpp>

#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

template <typename TVectorItem>
class SignalVectorModel : public QAbstractTableModel,
                          pajlada::Signals::SignalHolder
{
public:
    SignalVectorModel(int columnCount, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
        , columnCount_(columnCount)
    {
        for (int i = 0; i < columnCount; i++)
        {
            this->headerData_.emplace_back();
        }
    }

    void init(BaseSignalVector<TVectorItem> *vec)
    {
        this->vector_ = vec;

        auto insert = [this](const SignalVectorItemArgs<TVectorItem> &args) {
            if (args.caller == this)
            {
                return;
            }
            // get row index
            int index = this->getModelIndexFromVectorIndex(args.index);
            assert(index >= 0 && index <= this->rows_.size());

            // get row items
            std::vector<QStandardItem *> row = this->createRow();
            this->getRowFromItem(args.item, row);

            // insert row
            index = this->beforeInsert(args.item, row, index);

            this->beginInsertRows(QModelIndex(), index, index);
            this->rows_.insert(this->rows_.begin() + index,
                               Row(row, args.item));
            this->endInsertRows();
        };

        int i = 0;
        for (const TVectorItem &item : vec->getVector())
        {
            SignalVectorItemArgs<TVectorItem> args{item, i++, 0};

            insert(args);
        }

        this->managedConnect(vec->itemInserted, insert);

        this->managedConnect(vec->itemRemoved, [this](auto args) {
            if (args.caller == this)
            {
                return;
            }

            int row = this->getModelIndexFromVectorIndex(args.index);
            assert(row >= 0 && row <= this->rows_.size());

            // remove row
            std::vector<QStandardItem *> items =
                std::move(this->rows_[row].items);

            this->beginRemoveRows(QModelIndex(), row, row);
            this->rows_.erase(this->rows_.begin() + row);
            this->endRemoveRows();

            this->afterRemoved(args.item, items, row);

            for (QStandardItem *item : items)
            {
                delete item;
            }
        });

        this->afterInit();
    }

    virtual ~SignalVectorModel()
    {
        for (Row &row : this->rows_)
        {
            for (QStandardItem *item : row.items)
            {
                delete item;
            }
        }
    }

    int rowCount(const QModelIndex &parent) const override
    {
        (void)parent;

        return this->rows_.size();
    }

    int columnCount(const QModelIndex &parent) const override
    {
        (void)parent;

        return this->columnCount_;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        int row = index.row(), column = index.column();
        if (row < 0 || column < 0 || row >= this->rows_.size() ||
            column >= this->columnCount_)
        {
            return QVariant();
        }

        return rows_[row].items[column]->data(role);
    }

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role) override
    {
        int row = index.row(), column = index.column();
        if (row < 0 || column < 0 || row >= this->rows_.size() ||
            column >= this->columnCount_)
        {
            return false;
        }

        Row &rowItem = this->rows_[row];

        rowItem.items[column]->setData(value, role);

        if (rowItem.isCustomRow)
        {
            this->customRowSetData(rowItem.items, column, value, role, row);
        }
        else
        {
            int vecRow = this->getVectorIndexFromModelIndex(row);
            this->vector_->removeItem(vecRow, this);

            assert(this->rows_[row].original);
            TVectorItem item = this->getItemFromRow(
                this->rows_[row].items, this->rows_[row].original.get());
            this->vector_->insertItem(item, vecRow, this);
        }

        return true;
    }

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override
    {
        if (orientation != Qt::Horizontal)
        {
            return QVariant();
        }

        auto it = this->headerData_[section].find(role);
        if (it == this->headerData_[section].end())
        {
            return QVariant();
        }
        else
        {
            return it.value();
        }
    }

    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value,
                       int role = Qt::DisplayRole) override
    {
        if (orientation != Qt::Horizontal)
        {
            return false;
        }

        this->headerData_[section][role] = value;

        emit this->headerDataChanged(Qt::Horizontal, section, section);
        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        int row = index.row(), column = index.column();

        if (row < 0 || column < 0 || row >= this->rows_.size() ||
            column >= this->columnCount_)
        {
            return Qt::NoItemFlags;
        }

        assert(row >= 0 && row < this->rows_.size() && column >= 0 &&
               column < this->columnCount_);

        return this->rows_[row].items[column]->flags();
    }

    QStandardItem *getItem(int row, int column)
    {
        assert(row >= 0 && row < this->rows_.size() && column >= 0 &&
               column < this->columnCount_);

        return rows_[row].items[column];
    }

    void deleteRow(int row)
    {
        int signalVectorRow = this->getVectorIndexFromModelIndex(row);
        this->vector_->removeItem(signalVectorRow);
    }

    bool removeRows(int row, int count, const QModelIndex &parent) override
    {
        (void)parent;

        if (count != 1)
        {
            return false;
        }

        assert(row >= 0 && row < this->rows_.size());

        int signalVectorRow = this->getVectorIndexFromModelIndex(row);
        this->vector_->removeItem(signalVectorRow);

        return true;
    }

    QStringList mimeTypes() const override
    {
        return {"chatterino_row_id"};
    }

    QMimeData *mimeData(const QModelIndexList &list) const
    {
        if (list.length() == 1)
        {
            return nullptr;
        }

        // Check if all indices are in the same row -> single row selected
        for (auto &&x : list)
        {
            if (x.row() != list.first().row())
                return nullptr;
        }

        auto data = new QMimeData;
        data->setData("chatterino_row_id", QByteArray::number(list[0].row()));
        return data;
    }

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/,
                      int /*column*/, const QModelIndex &parent) override
    {
        if (data->hasFormat("chatterino_row_id") &&
            action & (Qt::DropAction::MoveAction | Qt::DropAction::CopyAction))
        {
            int from = data->data("chatterino_row_id").toInt();
            int to = parent.row();

            if (from < 0 || from > this->vector_->getVector().size() ||
                to < 0 || to > this->vector_->getVector().size())
            {
                return false;
            }

            if (from != to)
            {
                auto item = this->vector_->getVector()[from];
                this->vector_->removeItem(from);
                this->vector_->insertItem(item, to);
            }

            // We return false since we remove items ourselves.
            return false;
        }

        return false;
    }

    Qt::DropActions supportedDropActions() const override
    {
        return this->vector_->isSorted()
                   ? Qt::DropActions()
                   : Qt::DropAction::CopyAction | Qt::DropAction::MoveAction;
    }

protected:
    virtual void afterInit()
    {
    }

    // turn a vector item into a model row
    virtual TVectorItem getItemFromRow(std::vector<QStandardItem *> &row,
                                       const TVectorItem &original) = 0;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const TVectorItem &item,
                                std::vector<QStandardItem *> &row) = 0;

    virtual int beforeInsert(const TVectorItem &item,
                             std::vector<QStandardItem *> &row,
                             int proposedIndex)
    {
        (void)item, (void)row;

        return proposedIndex;
    }

    virtual void afterRemoved(const TVectorItem &item,
                              std::vector<QStandardItem *> &row, int index)
    {
        (void)item, (void)row, (void)index;
    }

    virtual void customRowSetData(const std::vector<QStandardItem *> &row,
                                  int column, const QVariant &value, int role,
                                  int rowIndex)
    {
        (void)row, (void)column, (void)value, (void)role, (void)rowIndex;
    }

    void insertCustomRow(std::vector<QStandardItem *> row, int index)
    {
        assert(index >= 0 && index <= this->rows_.size());

        this->beginInsertRows(QModelIndex(), index, index);
        this->rows_.insert(this->rows_.begin() + index,
                           Row(std::move(row), true));
        this->endInsertRows();
    }

    void removeCustomRow(int index)
    {
        assert(index >= 0 && index <= this->rows_.size());
        assert(this->rows_[index].isCustomRow);

        this->beginRemoveRows(QModelIndex(), index, index);
        this->rows_.erase(this->rows_.begin() + index);
        this->endRemoveRows();
    }

    std::vector<QStandardItem *> createRow()
    {
        std::vector<QStandardItem *> row;
        for (int i = 0; i < this->columnCount_; i++)
        {
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

private:
    std::vector<QMap<int, QVariant>> headerData_;
    BaseSignalVector<TVectorItem> *vector_;
    std::vector<Row> rows_;

    int columnCount_;

    // returns the related index of the SignalVector
    int getVectorIndexFromModelIndex(int index)
    {
        int i = 0;

        for (auto &row : this->rows_)
        {
            if (row.isCustomRow)
            {
                index--;
                continue;
            }

            if (i == index)
            {
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

        for (auto &row : this->rows_)
        {
            if (row.isCustomRow)
            {
                index++;
            }

            if (i == index)
            {
                return i;
            }
            i++;
        }

        return i;
    }
};

}  // namespace chatterino
