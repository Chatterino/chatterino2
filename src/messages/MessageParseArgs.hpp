// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

namespace chatterino {

struct MessageParseArgs {
    bool disablePingSounds = false;
    bool isReceivedWhisper = false;
    bool isSentWhisper = false;
    bool trimSubscriberUsername = false;
    bool isSubscriptionMessage = false;
    bool allowIgnore = true;
    bool isAction = false;
    QString channelPointRewardId = "";
};

}  // namespace chatterino
