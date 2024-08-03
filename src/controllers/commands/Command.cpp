#include "controllers/commands/Command.hpp"

namespace chatterino {

// command
Command::Command(const QString &_text)
{
    int index = _text.indexOf(' ');

    if (index == -1)
    {
        this->name = _text;
        return;
    }

    this->name = _text.mid(0, index).trimmed();
    this->func = _text.mid(index + 1).trimmed();
    this->showInMsgContextMenu = false;
}

Command::Command(const QString &_name, const QString &_func,
                 bool _showInMsgContextMenu)
    : name(_name.trimmed())
    , func(_func.trimmed())
    , showInMsgContextMenu(_showInMsgContextMenu)
{
}

QString Command::toString() const
{
    return this->name + " " + this->func;
}

}  // namespace chatterino
