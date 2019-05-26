# Environment variables
Below I have tried to list all environment variables that can be used to modify the behaviour of Chatterino. Used for things that I don't feel like fit in the settings system.

## CHATTERINO2_RECENT_MESSAGES_URL
Used to change the URL that Chatterino2 uses when trying to load historic Twitch chat messages (if the setting is enabled).  
Default value: `"https://recent-messages.robotty.de/api/v2/recent-messages/%1?clearchatToNotice=true"`  
Arguments:  
1) `%1` = Name of the Twitch channel
