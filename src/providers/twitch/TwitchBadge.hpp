// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "messages/MessageElement.hpp"

#include <QString>

namespace chatterino {

class TwitchBadge
{
public:
    TwitchBadge(QString key, QString value);

    bool operator==(const TwitchBadge &other) const;

    // Class members are fetched from both "badges" and "badge-info" tags
    // E.g.: "badges": "subscriber/18", "badge-info": "subscriber/22"
    QString key_;    // subscriber
    QString value_;  // 18
    //QString info_; // 22 (should be parsed separetly into an std::unordered_map)
    MessageElementFlag flag_{
        MessageElementFlag::BadgeVanity};  // badge slot it takes up
};

}  // namespace chatterino
