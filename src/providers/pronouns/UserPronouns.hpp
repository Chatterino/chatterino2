// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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
        if (this->isUnspecified())
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
