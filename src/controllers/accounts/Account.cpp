#include "Account.hpp"

#include <tuple>

namespace chatterino {
namespace controllers {
namespace accounts {

Account::Account(ProviderId _providerId)
    : providerId(_providerId)
{
    static QString twitch("Twitch");

    this->category = [&]() {
        switch (_providerId) {
            case ProviderId::Twitch:
                return twitch;
        }
        return QString("Unknown ProviderId");
    }();
}

const QString &Account::getCategory() const
{
    return this->category;
}

ProviderId Account::getProviderId() const
{
    return this->providerId;
}

bool Account::operator<(const Account &other) const
{
    QString a = this->toString();
    QString b = other.toString();

    return std::tie(this->category, a) < std::tie(other.category, b);
}

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
