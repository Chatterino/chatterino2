#pragma once

#include <QString>

namespace chatterino {
namespace controllers {
namespace accounts {

class Account
{
public:
    Account(const QString &category);

    virtual QString toString() const = 0;
    const QString &getCategory() const;

    bool operator<(const Account &other) const;

private:
    QString category;
};

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
