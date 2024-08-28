#include "providers/pronouns/UserPronouns.hpp"

#include <QString>

#include <optional>

namespace chatterino::pronouns {

UserPronouns::UserPronouns(QString pronouns)
    : representation{pronouns.length() != 0 ? std::move(pronouns) : QString()}
{
}

bool UserPronouns::isUnspecified() const
{
    return this->representation.isNull();
}

UserPronouns::operator bool() const
{
    return !this->representation.isNull();
}

}  // namespace chatterino::pronouns
