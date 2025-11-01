#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "messages/Link.hpp"
#    include "messages/Message.hpp"

#    include <sol/forward.hpp>

#    include <cstdint>

namespace chatterino::lua::api::message {

/* @lua-fragment
---A chat message
---@class c2.Message
c2.Message = {}

---A table to initialize a new message
---@class MessageInit
---@field flags? c2.MessageFlag Message flags (see `c2.MessageFlags`)
---@field id? string The (ideally unique) message ID
---@field parse_time? number Time the message was parsed (in milliseconds since epoch)
---@field search_text? string Text to that is compared when searching for messages
---@field message_text? string The message text (used for filters for example)
---@field login_name? string The login name of the sender
---@field display_name? string The display name of the sender
---@field localized_name? string The localized name of the sender (this is used for CJK names, otherwise it's empty)
---@field user_id? string The ID of the user who sent the message
---@field channel_name? string The name of the channel this message appeared in
---@field username_color? string The color of the username
---@field server_received_time? number The time the server received the message (in milliseconds since epoch)
---@field highlight_color? string|nil The color of the highlight (if any)
---@field elements? MessageElementInit[] The elements of the message

---A base table to initialize a new message element
---@class MessageElementInitBase
---@field tooltip? string Tooltip text
---@field trailing_space? boolean Whether to add a trailing space after the element (default: true)
---@field link? c2.Link An action when clicking on this element. Mention and Link elements don't support this. They manage the link themselves.

---@alias MessageColor "text"|"link"|"system"|string A color for a text element - "text", "link", and "system" are special values that take the current theme into account

---A table to initialize a new message text element
---@class TextElementInit : MessageElementInitBase
---@field type "text" The type of the element
---@field text string The text of this element
---@field flags? c2.MessageElementFlag Message element flags (see `c2.MessageElementFlags`)
---@field color? MessageColor The color of the text
---@field style? c2.FontStyle The font style of the text

---A table to initialize a new message single-line text element
---@class SingleLineTextElementInit : MessageElementInitBase
---@field type "single-line-text" The type of the element
---@field text string The text of this element
---@field flags? c2.MessageElementFlag Message element flags (see `c2.MessageElementFlags`)
---@field color? MessageColor The color of the text
---@field style? c2.FontStyle The font style of the text

---A table to initialize a new mention element
---@class MentionElementInit : MessageElementInitBase
---@field type "mention" The type of the element
---@field display_name string The display name of the mentioned user
---@field login_name string The login name of the mentioned user
---@field fallback_color MessageColor The color of the element in case the "Colorize @usernames" is disabled
---@field user_color MessageColor The color of the element in case the "Colorize @usernames" is enabled

---A table to initialize a new timestamp element
---@class TimestampElementInit : MessageElementInitBase
---@field type "timestamp" The type of the element
---@field time number? The time of the timestamp (in milliseconds since epoch). If not provided, the current time is used.

---A table to initialize a new Twitch moderation element (all the custom moderation buttons)
---@class TwitchModerationElementInit : MessageElementInitBase
---@field type "twitch-moderation" The type of the element

---A table to initialize a new linebreak element
---@class LinebreakElementInit : MessageElementInitBase
---@field type "linebreak" The type of the element
---@field flags? c2.MessageElementFlag Message element flags (see `c2.MessageElementFlags`)

---A table to initialize a new reply curve element
---@class ReplyCurveElementInit : MessageElementInitBase
---@field type "reply-curve" The type of the element

---@alias MessageElementInit TextElementInit|SingleLineTextElementInit|MentionElementInit|TimestampElementInit|TwitchModerationElementInit|LinebreakElementInit|ReplyCurveElementInit

--- Creates a new message
---
---@param init MessageInit The message initialization table
---@return c2.Message msg The new message
function c2.Message.new(init) end
*/

/** @lua@alias c2.Link { type: c2.LinkType, value: string } A link on a message element. */

// We only want certain links to be usable
// Note: code dependant on this needs these values to be meaningful `Link::Type`s
/** @exposeenum c2.LinkType */
enum class ExposedLinkType : std::uint8_t {
    Url = Link::Type::Url,
    UserInfo = Link::Type::UserInfo,
    UserAction = Link::Type::UserAction,  // run a command/send message
    JumpToChannel = Link::Type::JumpToChannel,
    CopyToClipboard = Link::Type::CopyToClipboard,
    JumpToMessage = Link::Type::JumpToMessage,
    InsertText = Link::Type::InsertText,
};

/**
 * @includefile singletons/Fonts.hpp 
 * @includefile messages/MessageElement.hpp
 * @includefile messages/MessageFlag.hpp
 * @includefile common/enums/MessageContext.hpp
 */

/// Creates the c2.Message user type
void createUserType(sol::table &c2);

}  // namespace chatterino::lua::api::message

#endif
