#include "Irc2.hpp"

#include "common/SignalVectorModel.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

namespace {
    class Model : public SignalVectorModel<IrcConnection_>
    {
    public:
        Model(QObject *parent)
            : SignalVectorModel<IrcConnection_>(6, parent)
        {
        }

        // turn a vector item into a model row
        IrcConnection_ getItemFromRow(std::vector<QStandardItem *> &row,
                                      const IrcConnection_ &original)
        {
            return IrcConnection_{
                row[0]->data(Qt::EditRole).toString(),  // host
                row[1]->data(Qt::EditRole).toInt(),     // port
                row[2]->data(Qt::Checked).toBool(),     // ssl
                row[3]->data(Qt::EditRole).toString(),  // user
                row[4]->data(Qt::EditRole).toString(),  // nick
                row[5]->data(Qt::EditRole).toString(),  // password
                original.id,                            // id
            };
        }

        // turns a row in the model into a vector item
        void getRowFromItem(const IrcConnection_ &item,
                            std::vector<QStandardItem *> &row)
        {
            setStringItem(row[0], item.host);
            setStringItem(row[1], QString::number(item.port));
            setBoolItem(row[2], item.ssl);
            setStringItem(row[3], item.user);
            setStringItem(row[4], item.nick);
            setStringItem(row[5], item.password);
        }
    };
}  // namespace

static std::atomic_int currentId;

Irc::Irc()
{
}

IrcConnection_ IrcConnection_::unique()
{
    IrcConnection_ c;
    c.id = currentId++;
    c.port = 6697;
    return c;
}

QAbstractTableModel *Irc::newConnectionModel(QObject *parent)
{
    auto model = new Model(parent);
    model->init(&this->connections);
    return model;
}

Irc &Irc::getInstance()
{
    static Irc irc;
    return irc;
}

}  // namespace chatterino
