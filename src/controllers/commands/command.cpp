#include "command.hpp"

namespace chatterino {
namespace controllers {
namespace commands {

// command
Command::Command(const QString &_text)
{
    int index = _text.indexOf(' ');

    if (index == -1) {
        this->name = _text;
        return;
    }

    this->name = _text.mid(0, index);
    this->func = _text.mid(index + 1);
}

Command::Command(const QString &_name, const QString &_func)
    : name(_name)
    , func(_func)
{
}

QString Command::toString() const
{
    return this->name + " " + this->func;
}

}  // namespace commands
}  // namespace controllers
}  // namespace chatterino
