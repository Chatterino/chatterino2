#pragma once

#include "providerid.hpp"

#include <QString>

namespace chatterino {
namespace controllers {
namespace accounts {

class Account
{
public:
    Account(ProviderId providerId);
    virtual ~Account() = default;

    virtual QString toString() const = 0;
    const QString &getCategory() const;
    ProviderId getProviderId() const;

    bool operator<(const Account &other) const;

private:
    ProviderId providerId;
    QString category;
};

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
