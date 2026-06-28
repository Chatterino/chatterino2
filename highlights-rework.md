> If we go for a list that contains every single highlight (message, user, badge), we should have some icon to signify for each highlight what type of highlight it is.

There's currently some icons that we need to figure out licenses for, but there's a message, user, badge, "automated", and filter icon.

> CHECKBOX TO SPECIFY ENABLED/DISABLED (would gray out the rest of the fields)

It's currently configurable in the dialog, and doesn't gray things out but changes the text from Enabled/Disabled. Should something more happen to the highlight? Different icon?

> NAME
> Text "my phrase"
> Message matches regex "dfkjghdfkjghkjhfgdfdg"
> Message matches regex "dfkjghdfkjghkjhfgdfdg" in #forsen
> Message matches regex "dfkjghdfkjghkjhfgdfdg" by @pajlada in #forsen
>
> Message matches regex "\byour username\b"

Highlights kept their old "simplicity", grouping them into message/user/badge highlights. To allow for more customizable highlights, we allow Filter highlights which use the exact same logic as filters.

> Design considerations:
> Can we allow the UI somehow to accomodate a plugin that helps
> faciliate highlights (e.g. 1-man-spam highlight)

I believe this should be possible - I've left places somewhat open for more options that won't be super disruptive (e.g. the "Add" button is just a dropdown)

> Can we utilize filter (or filter-like patterns) for replacements i.e. for your username?

not quite but we're using filters for advanced shit

> Chatty's highlight edit dialog has a really cool test text feature, especially handy for regex testing.

I have not looked at implementing this, but I believe that's best left for a follow-up PR.

> Could we take it further to allow the user to customize an entire message from a specific username in a specific channel with specific badges?

Yes, with filter highlights.

UNRELATED: Can commands access "the right-clicked element" context? i.e. if I right-click an emote, could it access the emote name? emote copytext?
UNRELATED: Can we make a command that appends a given string to the user's input box?
UNRELATED: Copy emote name from context menu?

Issues to keep in mind:

- https://github.com/Chatterino/chatterino2/discussions/6686

```json
{
  "subHighlightColor": "#FF00FF"
}
```

->

```json
{
  "highlights": {
    "subhighlight": {
      "color": "#FF00FF"
    }
  }
}
```

BREAKING(?) changes!!:

- FirstMessage now also has the Highlighted flag
- Subscription messages now also have the Highlighted flag
