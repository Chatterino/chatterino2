#pragma once

#include "messages/MessageElement.hpp"

#include <QString>

namespace chatterino {

class Badge
{
public:
    Badge(QString key, QString value);

    QString key_;           // e.g. bits
    QString value_;         // e.g. 100
    QString extraValue_{};  // e.g. 5 (the number of months subscribed)
    MessageElementFlag flag_{
        MessageElementFlag::BadgeVanity};  // badge slot it takes up
};

}  // namespace chatterino
