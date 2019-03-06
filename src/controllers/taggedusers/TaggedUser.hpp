#pragma once

#include "common/ProviderId.hpp"

#include <QString>

namespace chatterino
{
    class TaggedUser
    {
    public:
        TaggedUser(
            ProviderId providerId, const QString& name, const QString& id);

        bool operator<(const TaggedUser& other) const;

        ProviderId getProviderId() const;
        QString getName() const;
        QString getId() const;

    private:
        ProviderId providerId_;
        QString name_;
        QString id_;
    };

}  // namespace chatterino
