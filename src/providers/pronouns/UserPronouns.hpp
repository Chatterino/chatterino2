#pragma once

#include <optional>
#include <string>

namespace chatterino {

class UserPronouns
{
private:
    std::optional<std::string> representation;

public:
    UserPronouns();
    UserPronouns(std::string);

    inline std::string format() const
    {
        if (this->representation)
        {
            return *(this->representation);
        }
        else
        {
            return "unspecified";
        }
    }

    bool isUnspecified() const;

    // True, iff the pronouns are not unspecified.
    operator bool() const;
};

}  // namespace chatterino
