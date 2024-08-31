#pragma once

namespace chatterino {

// MessageOverflow is used for controlling how to guide the user into not
// sending a message that will be discarded by Twitch
enum MessageOverflow {
    // Allow overflowing characters to be inserted into the input box, but highlight them in red
    Highlight,

    // Prevent more characters from being inserted into the input box
    Prevent,

    // Do nothing
    Allow,
};

}  // namespace chatterino
