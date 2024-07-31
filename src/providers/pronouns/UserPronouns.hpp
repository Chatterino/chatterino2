#pragma once

#include <optional>
#include <string>

namespace chatterino {

class UserPronouns
{
private:
    std::optional<QString> representation;

public:
    UserPronouns() = default;
    UserPronouns(QString);

    inline QString format() const
    {
        if (this->representation)
        {
            return *(this->representation);
        }
        return "unspecified";
    }

    bool isUnspecified() const;

    // True, iff the pronouns are not unspecified.
    operator bool() const;
};

}  // namespace chatterino
