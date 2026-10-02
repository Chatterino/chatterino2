// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/ProviderId.hpp"

#include <QString>

namespace chatterino {

class Account
{
public:
    Account(ProviderId providerId);
    virtual ~Account() = default;

    virtual QString toString() const = 0;

    /// Whether this account's credentials are known to have expired.
    ///
    /// Providers that can detect this override it; the default is to assume
    /// the credentials are still good.
    virtual bool isExpired() const
    {
        return false;
    }

    const QString &getCategory() const;
    ProviderId getProviderId() const;

    bool operator<(const Account &other) const;

private:
    ProviderId providerId_;
    QString category_;
};

}  // namespace chatterino
