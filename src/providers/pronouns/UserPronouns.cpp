#include "providers/pronouns/UserPronouns.hpp"

#include <QString>

#include <optional>

namespace chatterino::pronouns {

UserPronouns::UserPronouns(QString pronouns)
    : representation{!pronouns.isEmpty() ? std::move(pronouns) : QString()}
{
}

bool UserPronouns::isUnspecified() const
{
    return this->representation.isEmpty();
}

UserPronouns::operator bool() const
{
    return !isUnspecified();
}

}  // namespace chatterino::pronouns
