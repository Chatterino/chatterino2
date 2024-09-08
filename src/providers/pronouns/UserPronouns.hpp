#pragma once

#include <QString>

namespace chatterino::pronouns {

class UserPronouns
{
public:
    UserPronouns() = default;
    UserPronouns(QString pronouns);

    QString format() const
    {
        if (isUnspecified())
        {
            return "unspecified";
        }
        return this->representation;
    }

    bool isUnspecified() const;

    /// True, iff the pronouns are not unspecified.
    operator bool() const;

private:
    QString representation;
};

}  // namespace chatterino::pronouns
