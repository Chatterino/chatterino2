# Twitch API
this folder describes what sort of API requests we do, what permissions are required for the requests etc

## Kraken (V5)
We use a bunch of Kraken (V5) in Chatterino2.

### Get Channel
URL: https://dev.twitch.tv/docs/v5/reference/channels#get-channel  

 * We use this API in `widgets/dialogs/UserInfoPopup.cpp updateUserData` to show the user ID of the user you clicked
 * We use this API in `providers/twitch/TwitchChannel.cpp refreshTitle` to check the current stream title/game

### Check user follows by Channel
URL: https://dev.twitch.tv/docs/v5/reference/users#check-user-follows-by-channel  

 * We use this API in `widgets/dialogs/UserInfoPopup.cpp installEvents` to show your follow status to the user whose name you just clicked to open the user info popup.
 * We implement this API in `providers/twitch/TwitchAccount.cpp checkFollow`

### Follow Channel
URL: https://dev.twitch.tv/docs/v5/reference/users#follow-channel  
Requires `user_follows_edit` scope

 * We implement this API in `providers/twitch/TwitchAccount.cpp followUser`

### Unfollow Channel
URL: https://dev.twitch.tv/docs/v5/reference/users#unfollow-channel  
Requires `user_follows_edit` scope

 * We implement this API in `providers/twitch/TwitchAccount.cpp unfollowUser`

### Get Stream by User
URL: https://dev.twitch.tv/docs/v5/reference/streams#get-stream-by-user  

 * We use this API in `controllers/notifications/NotificationController.cpp getFakeTwitchChannelLiveStatus` to provide notifications for channels you might now have open in Chatterino
 * We use this API in `providers/twitch/TwitchChannel refreshLiveStatus` to get the live status of a channel you have open in Chatterino

### Get User
URL: https://dev.twitch.tv/docs/v5/reference/users#get-user  

 * We implement this API in `providers/twitch/PartialTwitchUser.cpp getId`

### Get Cheermotes
URL: https://dev.twitch.tv/docs/v5/reference/bits#get-cheermotes  

 * We implement this API in `providers/twitch/TwitchChannel.cpp` to resolve a chats available cheer emotes. This helps us parse incoming messages like `pajaCheer1000`

### Get User Block List
URL: https://dev.twitch.tv/docs/v5/reference/users#get-user-block-list  

 * We use this in `providers/twitch/TwitchAccount.cpp loadIgnores`

### Block User
URL: https://dev.twitch.tv/docs/v5/reference/users#block-user  
Requires `user_blocks_edit` scope

 * We use this in `providers/twitch/TwitchAccount.cpp ignoreByID`

### Unblock User
URL: https://dev.twitch.tv/docs/v5/reference/users#unblock-user  
Requires `user_blocks_edit` scope

 * We use this in `providers/twitch/TwitchAccount.cpp unignoreByID`

### Get User Emotes
URL: https://dev.twitch.tv/docs/v5/reference/users#get-user-emotes  
Requires `user_subscriptions` scope

 * We use this in `providers/twitch/TwitchAccount.cpp loadEmotes` to figure out which emotes a user is allowed to use!

### AUTOMOD APPROVE
**Unofficial** documentation: https://discuss.dev.twitch.tv/t/allowing-others-aka-bots-to-use-twitchbot-reject/8508/2

 * We use this in `providers/twitch/TwitchAccount.cpp autoModAllow` to approve an automod deny/allow question

### AUTOMOD DENY
**Unofficial** documentation: https://discuss.dev.twitch.tv/t/allowing-others-aka-bots-to-use-twitchbot-reject/8508/2

 * We use this in `providers/twitch/TwitchAccount.cpp autoModDeny` to deny an automod deny/allow question

## Helix


### GET USERS

 * We implement this in `providers/twitch/api/Helix.cpp getUsers`.

   It is used in NotificationController and CommandController

## TMI
The TMI api is undocumented.

### Get Chatters
**Undocumented**

 * We use this in `widgets/splits/Split.cpp showViewerList`
 * We use this in `providers/twitch/TwitchChannel.cpp refreshChatters`
