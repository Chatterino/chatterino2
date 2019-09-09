#pragma once

#include <QString>
#include <functional>

namespace chatterino {

class Credentials
{
public:
    static Credentials &getInstance();

    QString get(const QString &provider, const QString &name,
                std::function<void(QString)> &&onLoaded);
    void set(const QString &provider, const QString &name,
             const QString &credential);
    void erase(const QString &provider, const QString &name);

private:
    Credentials();
};

}  // namespace chatterino
