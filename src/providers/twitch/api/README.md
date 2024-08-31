# Twitch API

this folder describes what sort of API requests we do, what permissions are required for the requests etc

## Helix

Full Helix API reference: https://dev.twitch.tv/docs/api/reference

### Adding support for a new endpoint

If you're adding support for a new endpoint, these are the things you should know.

1. Add a virtual function in the `IHelix` class. Naming should reflect the API name as best as possible.
1. Override the virtual function in the `Helix` class.
1. Mock the function in the `mock::Helix` class in the `mocks/include/mocks/Helix.hpp` file.
1. (Optional) Make a new error enum for the failure callback.

For a simple example, see the `updateUserChatColor` function and its error enum `HelixUpdateUserChatColorError`.
The API is used in the "/color" command in [CommandController.cpp](../../../controllers/commands/CommandController.cpp)

### Get Users

URL: https://dev.twitch.tv/docs/api/reference#get-users

Used in:

- `UserInfoPopup` to get ID, displayName, createdAt of username we clicked
- `CommandController` to power any commands that need to get a user ID
- `Toasts` to get the profile picture of a streamer who just went live
- `TwitchAccount` block and unblock features to translate user name to user ID

### Get Users Follows

URL: https://dev.twitch.tv/docs/api/reference#get-users-follows

Used in:

- `UserInfoPopup` to get number of followers a user has

### Get Streams

URL: https://dev.twitch.tv/docs/api/reference#get-streams

Used in:

- `LiveController` to get live status, game, title, and viewer count of a channel
- `NotificationController` to provide notifications for channels you might not have open in Chatterino, but are still interested in getting notifications for

### Create Clip

URL: https://dev.twitch.tv/docs/api/reference#create-clip  
Requires `clips:edit` scope

Used in:

- `TwitchChannel` to create a clip of a live broadcast

### Get Channel

URL: https://dev.twitch.tv/docs/api/reference#get-channel-information

Used in:

- `LiveController` to refresh stream title & display name

### Update Channel

URL: https://dev.twitch.tv/docs/api/reference#modify-channel-information  
Requires `channel:manage:broadcast` scope

Used in:

- `/setgame` to update the game in the current channel
- `/settitle` to update the title in the current channel

### Create Stream Marker

URL: https://dev.twitch.tv/docs/api/reference/#create-stream-marker  
Requires `user:edit:broadcast` scope

Used in:

- `controllers/commands/CommandController.cpp` in /marker command

### Get User Block List

URL: https://dev.twitch.tv/docs/api/reference#get-user-block-list  
Requires `user:read:blocked_users` scope

Used in:

- `providers/twitch/TwitchAccount.cpp loadBlocks` to load list of blocked (blocked) users by current user

### Block User

URL: https://dev.twitch.tv/docs/api/reference#block-user  
Requires `user:manage:blocked_users` scope

Used in:

- `widgets/dialogs/UserInfoPopup.cpp` to block a user via checkbox in the usercard
- `controllers/commands/CommandController.cpp` to block a user via "/block" command

### Unblock User

URL: https://dev.twitch.tv/docs/api/reference#unblock-user  
Requires `user:manage:blocked_users` scope

Used in:

- `widgets/dialogs/UserInfoPopup.cpp` to unblock a user via checkbox in the usercard
- `controllers/commands/CommandController.cpp` to unblock a user via "/unblock" command

### Search Categories

URL: https://dev.twitch.tv/docs/api/reference#search-categories

Used in:

- `controllers/commands/CommandController.cpp` in `/setgame` command to fuzzy search for game titles

### Manage Held AutoMod Messages

URL: https://dev.twitch.tv/docs/api/reference#manage-held-automod-messages  
Requires `moderator:manage:automod` scope

Used in:

- `providers/twitch/TwitchAccount.cpp` to approve/deny held AutoMod messages

### Get Cheermotes

URL: https://dev.twitch.tv/docs/api/reference/#get-cheermotes

Used in:

- `providers/twitch/TwitchChannel.cpp` to resolve a chats available cheer emotes. This helps us parse incoming messages like `pajaCheer1000`

### Get Global Badges

URL: https://dev.twitch.tv/docs/api/reference/#get-global-chat-badges

Used in:

- `providers/twitch/TwitchBadges.cpp` to load global badges

### Get Channel Badges

URL: https://dev.twitch.tv/docs/api/reference/#get-channel-chat-badges

Used in:

- `providers/twitch/TwitchChannel.cpp` to load channel badges

### Get Emote Sets

URL: https://dev.twitch.tv/docs/api/reference#get-emote-sets

Not used anywhere at the moment. Could be useful in the future for loading emotes from Helix.

### Get Channel Emotes

URL: https://dev.twitch.tv/docs/api/reference#get-channel-emotes

Not used anywhere at the moment.

### Get Chatters

URL: https://dev.twitch.tv/docs/api/reference/#get-chatters

Used for the chatter list for moderators/broadcasters.

### Send Shoutout

URL: https://dev.twitch.tv/docs/api/reference/#send-a-shoutout

Used in:

- `controllers/commands/CommandController.cpp` to send Twitch native shoutout using "/shoutout <username>"

### Warn Chat User

URL: https://dev.twitch.tv/docs/api/reference/#warn-chat-user

Used in:

- `controllers/commands/CommandController.cpp` to warn users via "/warn" command

## PubSub

### Whispers

We listen to the `whispers.<user_id>` PubSub topic to receive information about incoming whispers to the user

The EventSub alternative (`user.whisper.message`) is not yet implemented.

### Chat Moderator Actions

We listen to the `chat_moderator_actions.<user_id>.<channel_id>` PubSub topic to receive information about incoming moderator events in a channel.

We listen to this topic in every channel the user is a moderator.

We have not yet migrated to the EventSub equivalent topics:

- For showing bans & timeouts => `channel.moderate`
- For showing unbans & untimeouts => `channel.moderate`
- Clear/delete message => `channel.moderate`
- Roomstate (slow(off), followers(off), r9k(off), emoteonly(off), subscribers(off)) => `channel.moderate`
- VIP/Moderator added/removed => `channel.moderate`
- Raid started/cancelled => `channel.moderate`
- Add/delete permitted/blocked term => `channel.moderate` (or `automod.terms.update`)
- Modified automod properties => `automod.settings.update`
- Approve/deny unban request => `channel.moderate` (or `channel.unban_request.resolve`)

### AutoMod Queue

We listen to the `automod-queue.<moderator_id>.<channel_id>` PubSub topic to receive information about incoming automod events in a channel.

We listen to this topic in every channel the user is a moderator.

The EventSub alternative (`automod.message.hold` and `automod.message.update`) is not yet implemented.

### Channel Point Rewards

We listen to the `community-points-channel-v1.<channel_id>` PubSub topic to receive information about incoming channel points redemptions in a channel.

The EventSub alternative requires broadcaster auth, which is not a feasible alternative.

### Low Trust Users

We want to listen to the `low-trust-users` PubSub topic to receive information about messages from users who are marked as low-trust.

The EventSub alternative (`channel.suspicious_user.message` and `channel.suspicious_user.update`) is not yet implemented.
