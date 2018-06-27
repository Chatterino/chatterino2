#pragma once

#include "common/ProviderId.hpp"

#include <QString>

namespace chatterino {

class TaggedUser
{
public:
    TaggedUser(ProviderId provider, const QString &name, const QString &id);

    bool operator<(const TaggedUser &other) const;

    ProviderId provider;
    QString name;
    QString id;
};

}  // namespace chatterino
