#pragma once

namespace chatterino {
namespace messages {

struct MessageParseArgs {
    bool disablePingSounds = false;
    bool isReceivedWhisper = false;
    bool isSentWhisper = false;
};

}  // namespace messages
}  // namespace chatterino
