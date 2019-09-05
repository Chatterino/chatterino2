# Environment variables
Below I have tried to list all environment variables that can be used to modify the behaviour of Chatterino. Used for things that I don't feel like fit in the settings system.

### CHATTERINO2_RECENT_MESSAGES_URL
Used to change the URL that Chatterino2 uses when trying to load historic Twitch chat messages (if the setting is enabled).  
Default value: `https://recent-messages.robotty.de/api/v2/recent-messages/%1?clearchatToNotice=true`  
Arguments:  
 - `%1` = Name of the Twitch channel

### CHATTERINO2_LINK_RESOLVER_URL
Used to change the URL that Chatterino2 uses when trying to get link information to display in the tooltip on hover.  
Default value: `https://braize.pajlada.com/chatterino/link_resolver/%1`  
Arguments:  
 - `%1` = Escaped URL the link resolver should resolve

### CHATTERINO2_TWITCH_EMOTE_SET_RESOLVER_URL
Used to change the URL that Chatterino2 uses when trying to get emote set information
Default value: `https://braize.pajlada.com/chatterino/twitchemotes/set/%1/`  
Arguments:  
 - `%1` = Emote set ID

### CHATTERINO2_IMAGE_PASTE_SITE_URL
Used to change the URL that Chatterino2 uses when uploading an image by pasting it into the input box.
Default value: `https://i.nuuls.com/upload`
Arguments:
 - None

Notes:
 - The server that's running the web page MUST be compatible with [Nuuls' filehost](https://github.com/nuuls/filehost)
