#include "providers/pronouns/UserPronouns.hpp"

#include <optional>

namespace chatterino {

UserPronouns::UserPronouns(QString pronouns)
{
    if (pronouns.length() == 0)
    {
        this->representation = {};
    }
    else
    {
        this->representation = {std::move(pronouns)};
    }
}

bool UserPronouns::isUnspecified() const
{
    return !this->representation.has_value();
}

UserPronouns::operator bool() const
{
    return this->representation.has_value();
}

}  // namespace chatterino
