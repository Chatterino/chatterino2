#pragma once

#include <QMap>
#include <QString>

#include <memory>
#include <mutex>

#include <util/signalvector2.hpp>
#include <util/signalvectormodel.hpp>

namespace chatterino {
class Channel;

namespace singletons {

class CommandManager;

struct Command {
    QString name;
    QString func;

    Command() = default;
    explicit Command(const QString &text);
    Command(const QString &name, const QString &func);

    QString toString() const;
};

class CommandModel : public util::SignalVectorModel<Command>
{
    explicit CommandModel(QObject *parent);

protected:
    virtual int prepareVectorInserted(const Command &item, int index,
                                      std::vector<QStandardItem *> &rowToAdd) override;
    virtual int prepareVectorRemoved(const Command &item, int index) override;
    virtual int prepareModelItemRemoved(int index) override;

    friend class CommandManager;
};

//
// this class managed the custom /commands
//
class CommandManager
{
public:
    CommandManager();

    QString execCommand(const QString &text, std::shared_ptr<Channel> channel, bool dryRun);

    void load();
    void save();

    CommandModel *createModel(QObject *parent);

    util::UnsortedSignalVector<Command> items;

private:
    QMap<QString, Command> commandsMap;

    std::mutex mutex;
    QString filePath;

    QString execCustomCommand(const QStringList &words, const Command &command);
};

}  // namespace singletons
}  // namespace chatterino
