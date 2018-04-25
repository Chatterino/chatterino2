#pragma once

#include <utility>
#include <vector>

#include <QAbstractTableModel>

#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace util {

namespace {

template <int I>
struct TupleConverter {
    template <typename... Args>
    static void tupleToVariants(const std::tuple<Args...> &t, std::vector<QVariant> &row)
    {
        row[I - 1] = QVariant(std::get<I - 1>(t));
        TupleConverter<I - 1>::tupleToVariants<Args...>(t, row);
    }

    template <typename... Args>
    static void variantsToTuple(std::vector<QVariant> &row, std::tuple<Args...> &t)
    {
        std::get<I - 1>(t) = (decltype(std::get<I - 1>(t))) row[I - 1];
        TupleConverter<I - 1>::variantsToTuple<Args...>(row, t);
    }
};

template <>
struct TupleConverter<0> {
    template <typename... Args>
    static void tupleToVariants(const std::tuple<Args...> &t, std::vector<QVariant> &row)
    {
    }

    template <typename... Args>
    static void variantsToTuple(std::vector<QVariant> &row, std::tuple<Args...> &t)
    {
    }
};

}  // namespace

template <typename... Args>
class TupleTableModel : public QAbstractTableModel
{
    std::vector<std::vector<QVariant>> rows;
    std::vector<QMap<int, QVariant>> titleData;

public:
    pajlada::Signals::NoArgSignal itemsChanged;

    TupleTableModel()
    {
        titleData.resize(sizeof...(Args));
    }

    void addRow(const std::tuple<Args...> &row)
    {
        this->beginInsertRows(QModelIndex(), this->rows.size(), this->rows.size());
        std::vector<QVariant> variants;
        variants.resize(sizeof...(Args));
        TupleConverter<sizeof...(Args)>::tupleToVariants<Args...>(row, variants);
        this->rows.push_back(variants);
        this->endInsertRows();
        this->itemsChanged.invoke();
    }

    void addRow(Args... args)
    {
        this->beginInsertRows(QModelIndex(), this->rows.size(), this->rows.size());
        std::vector<QVariant> variants;
        variants.resize(sizeof...(Args));
        TupleConverter<sizeof...(Args)>::tupleToVariants<Args...>(std::tuple<Args...>(args...),
                                                                  variants);
        this->rows.push_back(variants);
        this->endInsertRows();
        this->itemsChanged.invoke();
    }

    std::tuple<Args...> getRow(int index)
    {
        std::tuple<Args...> row;
        TupleConverter<sizeof...(Args)>::variantsToTuple<Args...>(this->rows[index], row);
        return row;
    }

    void removeRow(int index)
    {
        this->beginRemoveRows(QModelIndex(), index, index);
        this->rows.erase(this->rows.begin() + index);
        this->endRemoveRows();
        this->itemsChanged.invoke();
    }

    void setTitles(std::initializer_list<QString> titles)
    {
        int i = 0;

        for (const QString &title : titles) {
            this->setHeaderData(i++, Qt::Horizontal, title, Qt::DisplayRole);

            if (i >= sizeof...(Args))
                break;
        }
    }

    int getRowCount() const
    {
        return this->rows.size();
    }

protected:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return this->rows.size();
    }

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return sizeof...(Args);
    }

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        QVariant data = this->rows[index.row()][index.column()];

        switch (role) {
            case Qt::DisplayRole: {
                if (data.type() == QVariant::Bool)
                    return QVariant();
                else
                    return data;
            } break;
            case Qt::EditRole: {
                return data;
            } break;
            case Qt::CheckStateRole: {
                if (data.type() == QVariant::Bool)
                    return data;
                else
                    return QVariant();
            } break;
        }
        return QVariant();
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value,
                         int role = Qt::EditRole) override
    {
        QVariant data = this->rows[index.row()][index.column()];

        switch (role) {
            case (Qt::EditRole): {
                this->rows[index.row()][index.column()] = value;
                this->itemsChanged.invoke();
                return true;
            } break;
            case (Qt::CheckStateRole): {
                if (data.type() == QVariant::Bool) {
                    this->rows[index.row()][index.column()] = !data.toBool();
                    this->itemsChanged.invoke();
                    return true;
                }
            } break;
        }

        return false;
    }

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal)
            return QVariant();
        if (section < 0 || section >= sizeof...(Args))
            return QVariant();

        auto it = this->titleData[section].find(role);
        return it == this->titleData[section].end() ? QVariant() : it.value();
    }

    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                               int role)
    {
        if (orientation != Qt::Horizontal)
            return false;
        if (section < 0 || section >= sizeof...(Args))
            return false;

        this->titleData[section][role] = value;
        return true;
    }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    {
        QVariant data = this->rows[index.row()][index.column()];

        if (data.type() == QVariant::Bool) {
            return Qt::ItemIsUserCheckable | Qt::ItemIsEditable | Qt::ItemIsEnabled |
                   Qt::ItemIsSelectable;
        }

        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
};

}  // namespace util
}  // namespace chatterino
