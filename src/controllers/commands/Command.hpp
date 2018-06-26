#pragma once

#include <QString>

namespace chatterino {

struct Command {
    QString name;
    QString func;

    Command() = default;
    explicit Command(const QString &text);
    Command(const QString &name, const QString &func);

    QString toString() const;
};

}  // namespace chatterino
