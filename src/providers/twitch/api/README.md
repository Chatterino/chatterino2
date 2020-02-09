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

 * We use this API in `singletons/Toasts.cpp fetchChannelAvatar` to resolve a users avatar by their login name
 * We implement this API in `providers/twitch/TwitchApi.cpp findUserId`
 * We implement this API in `providers/twitch/TwitchApi.cpp findUserName`
 * We implement this API in `providers/twitch/PartialTwitchUser.cpp getId`

### Get Cheermotes
URL: https://dev.twitch.tv/docs/v5/reference/bits#get-cheermotes  

 * We implement this API in `providers/twitch/TwitchChannel.cpp` to resolve a chats available cheer emotes. This helps us parse incoming messages like `pajaCheer1000`

### Get User Block List
URL: https://dev.twitch.tv/docs/v5/reference/users#get-user-block-list  

 * We use this in `providers/twitch/TwitchAccount.cpp loadIgnores`

### BLOCK USER
URL: XD

 * We use this in `providers/twitch/TwitchAccount.cpp ignoreByID`

### UNBLOCK USER
URL: XD

 * We use this in `providers/twitch/TwitchAccount.cpp unignoreByID`

### LOAD USER EMOTES
URL: XD

 * We use this in `providers/twitch/TwitchAccount.cpp loadEmotes` to figure out which emotes a user is allowed to use!

### AUTOMOD APPROVE
URL: XD

 * We use this in `providers/twitch/TwitchAccount.cpp autoModAllow` to approve an automod deny/allow question

### AUTOMOD DENY
URL: XD

 * We use this in `providers/twitch/TwitchAccount.cpp autoModDeny` to deny an automod deny/allow question

## Helix
We don't currently use Helix in Chatterino2.

## TMI
The TMI api is undocumented.

### Get Chatters
This endpoint is *undocumented*.

 * We use this in `widgets/splits/Split.cpp showViewerList`
 * We use this in `providers/twitch/TwitchChannel.cpp refreshChatters`
