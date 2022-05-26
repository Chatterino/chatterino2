#pragma once

#include "messages/MessageElement.hpp"

#include <QString>

namespace chatterino {

class Badge
{
public:
    Badge(const std::pair<const QString, QString> keyValuePair);

    QString key_;    // e.g. bits
    QString value_;  // e.g. 100
    MessageElementFlag flag_{
        MessageElementFlag::BadgeVanity};  // badge slot it takes up
};

}  // namespace chatterino
