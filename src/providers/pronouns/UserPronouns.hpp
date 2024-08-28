#pragma once

#include <QString>

#include <optional>
#include <string>

namespace chatterino::pronouns {

class UserPronouns
{
public:
    UserPronouns() = default;
    UserPronouns(QString);

    inline QString format() const
    {
        if (!this->representation.isNull())
        {
            return this->representation;
        }
        return "unspecified";
    }

    bool isUnspecified() const;

    // True, iff the pronouns are not unspecified.
    operator bool() const;

private:
    QString representation;
};

}  // namespace chatterino
