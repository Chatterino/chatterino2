#pragma once

#include <QString>
#include <vector>

namespace chatterino {

class CommandManager
{
    CommandManager() = default;

public:
    static CommandManager &getInstance();

    //    CommandManager() = delete;

    //    QString execCommand(QString text);
    // void addCommand ?
    // void loadCommands(QString) taking all commands as a \n seperated list ?

    //    static CommandManager *getInstance()
    //    {
    //        static CommandManager manager;

    //        return manager;
    //    }

    // private:
    //    struct Command {
    //        QString name;
    //        QString text;

    //        Command(QString text);
    //    };

    //    std::vector<Command> commands;
};
}
