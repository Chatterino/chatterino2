#pragma once

#include "common/ProviderId.hpp"

#include <QString>

namespace chatterino
{
    class Account
    {
    public:
        Account(ProviderId providerId);
        virtual ~Account() = default;

        virtual QString toString() const = 0;
        const QString& getCategory() const;
        ProviderId getProviderId() const;

        bool operator<(const Account& other) const;

    private:
        ProviderId providerId_;
        QString category_;
    };
}  // namespace chatterino
