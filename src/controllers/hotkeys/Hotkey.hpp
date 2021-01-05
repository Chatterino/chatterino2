#pragma once

#include "HotkeyScope.hpp"

#include <QString>

namespace chatterino {

class Hotkey
{
public:
    Hotkey(HotkeyScope scope, QKeySequence keySequence, QString action);
    virtual ~Hotkey() = default;

    QString toString() const;
    const QKeySequence &keySequence() const;
    HotkeyScope scope() const;
    QString action() const;
    QStringList arguments() const;
    QString getCategory() const;
    Qt::ShortcutContext getContext() const;

private:
    HotkeyScope scope_;
    QKeySequence keySequence_;
    QString action_;
    QStringList arguments_;
};

}  // namespace chatterino
