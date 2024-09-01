#include "TestModel.hpp"

namespace chatterino {

bool TestModel::setData(const QModelIndex &index, const QVariant &value,
                        int role)
{
    int row = index.row();
    int column = index.column();
    if (row < 0 || column < 0 || row >= this->rows_.size() ||
        column >= this->columnCount_)
    {
        return false;
    }

    Row &rowItem = this->rows_[row];

    assert(this->columnCount_ == rowItem.items.size());

    auto &cell = rowItem.items[column];

    cell->setData(value, role);

    if (rowItem.isCustomRow)
    {
        this->customRowSetData(rowItem.items, column, value, role, row);
    }
    else
    {
        int vecRow = this->getVectorIndexFromModelIndex(row);
        // TODO: This is only a safety-thing for when we modify data that's being modified right now.
        // It should not be necessary, but it would require some rethinking about this surrounding logic
        if (vecRow >= this->vector_->readOnly()->size())
        {
            return false;
        }
        this->vector_->removeAt(vecRow, this);

        assert(this->rows_[row].original);
        TVectorItem item = this->getItemFromRow(
            this->rows_[row].items, this->rows_[row].original.value());
        this->vector_->insert(item, vecRow, this);

        QVector<int> roles = QVector<int>();
        roles.append(role);
        emit dataChanged(index, index, roles);
    }

    return true;
}

}  // namespace chatterino
