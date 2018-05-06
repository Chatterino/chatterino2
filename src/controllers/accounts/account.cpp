#include "account.hpp"

namespace chatterino {
namespace controllers {
namespace accounts {

Account::Account(const QString &category)
{
}

const QString &Account::getCategory() const
{
    return this->category;
}

bool Account::operator<(const Account &other) const
{
    if (this->category < other.category) {
        return true;
    } else if (this->category == other.category) {
        if (this->toString() < other.toString()) {
            return true;
        }
    }
    return false;
}

}  // namespace accounts
}  // namespace controllers
}  // namespace chatterino
