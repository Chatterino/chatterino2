#pragma once

#include "messages/MessageElement.hpp"

#include <QString>

namespace chatterino {

class Badge
{
public:
    Badge(QString key, QString value);

    bool operator==(const Badge &other) const;

    // Class members are fetched from both "badges" and "badge-info" tags
    // E.g.: "badges": "subscriber/18", "badge-info": "subscriber/22"
    QString key_;    // subscriber
    QString value_;  // 18
    //QString info_; // 22 (should be parsed separetly into an std::unordered_map)
    MessageElementFlag flag_{
        MessageElementFlag::BadgeVanity};  // badge slot it takes up
};

}  // namespace chatterino
