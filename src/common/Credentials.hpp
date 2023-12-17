#pragma once

#include <QObject>
#include <QString>

#include <functional>

namespace chatterino {

class Credentials
{
public:
    static Credentials &instance();

    void get(const QString &provider, const QString &name, QObject *receiver,
             std::function<void(const QString &)> &&onLoaded);
    void set(const QString &provider, const QString &name,
             const QString &credential);
    void erase(const QString &provider, const QString &name);

private:
    Credentials() = default;
};

}  // namespace chatterino
