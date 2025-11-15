If we go for a list that contains every single highlight (message, user, badge), we should have some icon to signify for each highlight what type of highlight it is.

ICON TO SPECIFY TYPE:
Message/Text highlight
User highlight (user icon)
Badge highlight (badge icon)
Advanced highlight (i.e. badge + user + message?)

CHECKBOX TO SPECIFY ENABLED/DISABLED (would gray out the rest of the fields)

NAME
Text "my phrase"
Message matches regex "dfkjghdfkjghkjhfgdfdg"
Message matches regex "dfkjghdfkjghkjhfgdfdg" in #forsen
Message matches regex "dfkjghdfkjghkjhfgdfdg" by @pajlada in #forsen

Message matches regex "\byour username\b"

COLOR



Design considerations:
Can we allow the UI somehow to accomodate a plugin that helps
faciliate highlights (e.g. 1-man-spam highlight)

Can we utilize filter (or filter-like patterns) for replacements i.e. for your username?

Chatty's highlight edit dialog has a really cool test text feature, especially handy for regex testing.
Could we take it further to allow the user to customize an entire message from a specific username in a specific channel with specific badges?

UNRELATED: Can commands access "the right-clicked element" context? i.e. if I right-click an emote, could it access the emote name? emote copytext?
UNRELATED: Can we make a command that appends a given string to the user's input box?
UNRELATED: Copy emote name from context menu?
