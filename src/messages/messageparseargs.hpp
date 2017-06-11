#pragma once

namespace chatterino {
namespace messages {

struct MessageParseArgs {
public:
    bool disablePingSoungs = false;
    bool isReceivedWhisper = false;
    bool isSentWhisper = false;
    bool includeChannelName = false;
};

}  // namespace messages
}  // namespace chatterino
