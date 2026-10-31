#pragma once

#include <QObject>
#include <QString>

namespace chatterino::mock::lua {

struct MockQObject : public QObject {
    Q_OBJECT

Q_SIGNALS:
    void empty();
    void boolArg(bool b);                     // NOLINT(readability-*)
    void multiArg(const QString &qs, int i);  // NOLINT(readability-*)
};

}  // namespace chatterino::mock::lua
