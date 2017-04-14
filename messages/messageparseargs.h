#ifndef MESSAGEPARSEARGS_H
#define MESSAGEPARSEARGS_H

namespace  chatterino {
namespace  messages {

struct MessageParseArgs {
public:
    bool disablePingSoungs = false;
    bool isReceivedWhisper = false;
    bool isSentWhisper = false;
    bool includeChannelName = false;
};
}
}

#endif  // MESSAGEPARSEARGS_H
