> If we go for a list that contains every single highlight (message, user, badge), we should have some icon to signify for each highlight what type of highlight it is.

There's currently some icons that we need to figure out licenses for, but there's a message, user, badge, "automated", and filter icon.

> Chatty's highlight edit dialog has a really cool test text feature, especially handy for regex testing.

I have not looked at implementing this, but I believe that's best left for a follow-up PR.

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

NOTE: Should we add an option for highlights to short-circuit, meaning no further highlights should be executed?
NOTE: Add some more debug-entries

BREAKING(?) changes!!:

- FirstMessage now also has the Highlighted flag
- Subscription messages now also have the Highlighted flag
- The "highlight" order of ChannelPointsHighlight, FirstMessageHighlight, HypeChatHighlight, and WatchStreakHighlight are not the same as before. Maybe also Announcements if I get to those.
- There's no longer a way to keep a custom sound URL attached to a highlight while having the sound disabled. I intend to add a volume slider so you could achieve the same/something similar by setting the volume to 0

NOTE: If a user happens to have duplicate built-in highlights, should we clean that up on launch?
