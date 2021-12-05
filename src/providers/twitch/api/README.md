# Twitch API

this folder describes what sort of API requests we do, what permissions are required for the requests etc

## Kraken (V5)

We use few Kraken endpoints in Chatterino2.

### Get User Emotes

URL: https://dev.twitch.tv/docs/v5/reference/users#get-user-emotes  
Requires `user_subscriptions` scope

Migration path: **Unknown**

- We use this in `providers/twitch/TwitchAccount.cpp loadEmotes` to figure out which emotes a user is allowed to use!

## Helix

Full Helix API reference: https://dev.twitch.tv/docs/api/reference

### Get Users

URL: https://dev.twitch.tv/docs/api/reference#get-users

- We implement this in `providers/twitch/api/Helix.cpp fetchUsers`.  
  Used in:
  - `UserInfoPopup` to get ID, viewCount, displayName, createdAt of username we clicked
  - `CommandController` to power any commands that need to get a user ID
  - `Toasts` to get the profile picture of a streamer who just went live
  - `TwitchAccount` block and unblock features to translate user name to user ID

### Get Users Follows

URL: https://dev.twitch.tv/docs/api/reference#get-users-follows

- We implement this in `providers/twitch/api/Helix.cpp fetchUsersFollows`  
  Used in:
  - `UserInfoPopup` to get number of followers a user has

### Get Streams

URL: https://dev.twitch.tv/docs/api/reference#get-streams

- We implement this in `providers/twitch/api/Helix.cpp fetchStreams`  
  Used in:
  - `TwitchChannel` to get live status, game, title, and viewer count of a channel
  - `NotificationController` to provide notifications for channels you might not have open in Chatterino, but are still interested in getting notifications for

### Create Clip

URL: https://dev.twitch.tv/docs/api/reference#create-clip  
Requires `clips:edit` scope

- We implement this in `providers/twitch/api/Helix.cpp createClip`  
  Used in:
  - `TwitchChannel` to create a clip of a live broadcast

### Get Channel

URL: https://dev.twitch.tv/docs/api/reference#get-channel-information

- We implement this in `providers/twitch/api/Helix.cpp getChannel`  
  Used in:
  - `TwitchChannel` to refresh stream title

### Update Channel

URL: https://dev.twitch.tv/docs/api/reference#modify-channel-information  
Requires `channel:manage:broadcast` scope

- We implement this in `providers/twitch/api/Helix.cpp updateChannel`  
  Used in:
  - `/setgame` to update the game in the current channel
  - `/settitle` to update the title in the current channel

### Create Stream Marker

URL: https://dev.twitch.tv/docs/api/reference/#create-stream-marker  
Requires `user:edit:broadcast` scope

- We implement this in `providers/twitch/api/Helix.cpp createStreamMarker`  
  Used in:
  - `controllers/commands/CommandController.cpp` in /marker command

### Get User Block List

URL: https://dev.twitch.tv/docs/api/reference#get-user-block-list  
Requires `user:read:blocked_users` scope

- We implement this in `providers/twitch/api/Helix.cpp loadBlocks`  
  Used in:
  - `providers/twitch/TwitchAccount.cpp loadBlocks` to load list of blocked (blocked) users by current user

### Block User

URL: https://dev.twitch.tv/docs/api/reference#block-user  
Requires `user:manage:blocked_users` scope

- We implement this in `providers/twitch/api/Helix.cpp blockUser`  
  Used in:
  - `widgets/dialogs/UserInfoPopup.cpp` to block a user via checkbox in the usercard
  - `controllers/commands/CommandController.cpp` to block a user via "/block" command

### Unblock User

URL: https://dev.twitch.tv/docs/api/reference#unblock-user  
Requires `user:manage:blocked_users` scope

- We implement this in `providers/twitch/api/Helix.cpp unblockUser`  
  Used in:
  - `widgets/dialogs/UserInfoPopup.cpp` to unblock a user via checkbox in the usercard
  - `controllers/commands/CommandController.cpp` to unblock a user via "/unblock" command

### Search Categories

URL: https://dev.twitch.tv/docs/api/reference#search-categories

- We implement this in `providers/twitch/api/Helix.cpp searchGames`  
  Used in:
  - `controllers/commands/CommandController.cpp` in `/setgame` command to fuzzy search for game titles

### Manage Held AutoMod Messages

URL: https://dev.twitch.tv/docs/api/reference#manage-held-automod-messages  
Requires `moderator:manage:automod` scope

- We implement this in `providers/twitch/api/Helix.cpp manageAutoModMessages`  
  Used in:
  - `providers/twitch/TwitchAccount.cpp` to approve/deny held AutoMod messages

### Get Cheermotes

URL: https://dev.twitch.tv/docs/api/reference/#get-cheermotes

- We implement this in `providers/twitch/api/Helix.cpp getCheermotes`  
  Used in:
  - `providers/twitch/TwitchChannel.cpp` to resolve a chats available cheer emotes. This helps us parse incoming messages like `pajaCheer1000`

### Get Emote Sets

URL: https://dev.twitch.tv/docs/api/reference#get-emote-sets

- We implement this in `providers/twitch/api/Helix.cpp getEmoteSetData`  
  Used in:
  - `providers/twitch/TwitchAccount.cpp` to set emoteset owner data upon loading subscriber emotes from Kraken

### Get Channel Emotes

URL: https://dev.twitch.tv/docs/api/reference#get-channel-emotes

- We implement this in `providers/twitch/api/Helix.cpp getChannelEmotes`  
  Not used anywhere at the moment.

## TMI

The TMI api is undocumented.

### Get Chatters

**Undocumented**

- We use this in `widgets/splits/Split.cpp showViewerList`
- We use this in `providers/twitch/TwitchChannel.cpp refreshChatters`
