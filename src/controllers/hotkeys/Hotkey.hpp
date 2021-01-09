#pragma once

#include "HotkeyScope.hpp"

#include <QString>

namespace chatterino {

class Hotkey
{
public:
    Hotkey(HotkeyScope scope, QKeySequence keySequence, QString action,
           std::vector<QString> arguments, QString name);
    virtual ~Hotkey() = default;

    QString toString() const;
    const QKeySequence &keySequence() const;
    HotkeyScope scope() const;
    QString action() const;
    std::vector<QString> arguments() const;
    QString name() const;
    QString getCategory() const;
    Qt::ShortcutContext getContext() const;

private:
    HotkeyScope scope_;
    QKeySequence keySequence_;
    QString action_;
    std::vector<QString> arguments_;
    QString name_;
};

}  // namespace chatterino
