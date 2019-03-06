#include "Command.hpp"

namespace chatterino
{
    // command
    Command::Command(const QString& _text)
    {
        int index = _text.indexOf(' ');

        if (index == -1)
        {
            this->name = _text;
            return;
        }

        this->name = _text.mid(0, index).trimmed();
        this->func = _text.mid(index + 1).trimmed();
    }

    Command::Command(const QString& _name, const QString& _func)
        : name(_name.trimmed())
        , func(_func.trimmed())
    {
    }

    QString Command::toString() const
    {
        return this->name + " " + this->func;
    }

}  // namespace chatterino
