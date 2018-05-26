#include "account.hpp"

#include <tuple>

namespace chatterino {
namespace controllers {
namespace accounts {

Account::Account(const QString &_category)
    : category(_category)
{
}

const QString &Account::getCategory() const
{
    return this->category;
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
