# Twitch API
this folder describes what sort of API requests we do, what permissions are required for the requests etc

## Kraken (V5)
We use a bunch of Kraken (V5) in Chatterino2.

### Get Channel
URL: https://dev.twitch.tv/docs/v5/reference/channels#get-channel

Migration path: **Unknown**

 * We use this API in `providers/twitch/TwitchChannel.cpp refreshTitle` to check the current stream title/game

### Follow Channel
URL: https://dev.twitch.tv/docs/v5/reference/users#follow-channel  
Requires `user_follows_edit` scope

Migration path: **Unknown**

 * We implement this API in `providers/twitch/TwitchAccount.cpp followUser`

### Unfollow Channel
URL: https://dev.twitch.tv/docs/v5/reference/users#unfollow-channel  
Requires `user_follows_edit` scope

Migration path: **Unknown**

 * We implement this API in `providers/twitch/TwitchAccount.cpp unfollowUser`


### Get Cheermotes
URL: https://dev.twitch.tv/docs/v5/reference/bits#get-cheermotes

Migration path: **Not checked**

 * We implement this API in `providers/twitch/TwitchChannel.cpp` to resolve a chats available cheer emotes. This helps us parse incoming messages like `pajaCheer1000`

### Get User Block List
URL: https://dev.twitch.tv/docs/v5/reference/users#get-user-block-list

Migration path: **Unknown**

 * We use this in `providers/twitch/TwitchAccount.cpp loadIgnores`

### Block User
URL: https://dev.twitch.tv/docs/v5/reference/users#block-user  
Requires `user_blocks_edit` scope

Migration path: **Unknown**

 * We use this in `providers/twitch/TwitchAccount.cpp ignoreByID`

### Unblock User
URL: https://dev.twitch.tv/docs/v5/reference/users#unblock-user  
Requires `user_blocks_edit` scope

Migration path: **Unknown**

 * We use this in `providers/twitch/TwitchAccount.cpp unignoreByID`

### Get User Emotes
URL: https://dev.twitch.tv/docs/v5/reference/users#get-user-emotes  
Requires `user_subscriptions` scope

Migration path: **Unknown**

 * We use this in `providers/twitch/TwitchAccount.cpp loadEmotes` to figure out which emotes a user is allowed to use!

### AUTOMOD APPROVE
**Unofficial** documentation: https://discuss.dev.twitch.tv/t/allowing-others-aka-bots-to-use-twitchbot-reject/8508/2

 * We use this in `providers/twitch/TwitchAccount.cpp autoModAllow` to approve an automod deny/allow question

### AUTOMOD DENY
**Unofficial** documentation: https://discuss.dev.twitch.tv/t/allowing-others-aka-bots-to-use-twitchbot-reject/8508/2

 * We use this in `providers/twitch/TwitchAccount.cpp autoModDeny` to deny an automod deny/allow question

## Helix
Full Helix API reference: https://dev.twitch.tv/docs/api/reference

### Get Users
URL: https://dev.twitch.tv/docs/api/reference#get-users

 * We implement this in `providers/twitch/api/Helix.cpp fetchUsers`.  
   Used in:
     * `UserInfoPopup` to get ID and viewcount of username we clicked
     * `CommandController` to power any commands that need to get a user ID
     * `Toasts` to get the profile picture of a streamer who just went live
     * `TwitchAccount` ignore and unignore features to translate user name to user ID

### Get Users Follows
URL: https://dev.twitch.tv/docs/api/reference#get-users-follows

 * We implement this in `providers/twitch/api/Helix.cpp fetchUsersFollows`  
   Used in:
     * `UserInfoPopup` to get number of followers a user has

### Get Streams
URL: https://dev.twitch.tv/docs/api/reference#get-streams

 * We implement this in `providers/twitch/api/Helix.cpp fetchStreams`  
   Used in:
     * `TwitchChannel` to get live status, game, title, and viewer count of a channel
     * `NotificationController` to provide notifications for channels you might not have open in Chatterino, but are still interested in getting notifications for

## TMI
The TMI api is undocumented.

### Get Chatters
**Undocumented**

 * We use this in `widgets/splits/Split.cpp showViewerList`
 * We use this in `providers/twitch/TwitchChannel.cpp refreshChatters`
