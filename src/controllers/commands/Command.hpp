#pragma once

#include <QString>

namespace chatterino {
namespace controllers {
namespace commands {

struct Command {
    QString name;
    QString func;

    Command() = default;
    explicit Command(const QString &text);
    Command(const QString &name, const QString &func);

    QString toString() const;
};

}  // namespace commands
}  // namespace controllers
}  // namespace chatterino
