#include "providers/pronouns/UserPronouns.hpp"

#include <optional>
#include <string>

namespace chatterino {

UserPronouns::UserPronouns()
{
    representation = std::nullopt;
}
UserPronouns::UserPronouns(std::string pronouns)
{
    if (pronouns.length() == 0)
    {
        this->representation = {};
    }
    else
    {
        this->representation = {pronouns};
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
