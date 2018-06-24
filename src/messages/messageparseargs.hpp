#pragma once

namespace chatterino {
namespace messages {

struct MessageParseArgs {
    bool disablePingSounds = false;
    bool isReceivedWhisper = false;
    bool isSentWhisper = false;
    bool trimSubscriberUsername = false;
    bool isStaffOrBroadcaster = false;
};

}  // namespace messages
}  // namespace chatterino
