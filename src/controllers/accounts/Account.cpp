#include "controllers/accounts/Account.hpp"

#include <tuple>

namespace chatterino {

Account::Account(ProviderId providerId)
    : providerId_(providerId)
{
    static QString twitch("Twitch");

    this->category_ = [&]() {
        switch (providerId)
        {
            case ProviderId::Twitch:
                return twitch;
        }
        return QString("Unknown ProviderId");
    }();
}

const QString &Account::getCategory() const
{
    return this->category_;
}

ProviderId Account::getProviderId() const
{
    return this->providerId_;
}

bool Account::operator<(const Account &other) const
{
    QString a = this->toString();
    QString b = other.toString();

    return std::tie(this->category_, a) < std::tie(other.category_, b);
}

}  // namespace chatterino
