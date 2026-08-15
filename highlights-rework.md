NOTE: Should we add an option for highlights to short-circuit, meaning no further highlights should be executed?
NOTE: Add some more debug-entries
NOTE: When adding a highlight under Settings -> Highlights -> Add, there's no icon for the "Badge highlight". In the model/list itself, we use whatever Twitch badge exists, but I'd prefer to use a generic badge in the Add menu.

BREAKING(?) changes!!:

- FirstMessage now also has the Highlighted flag
- Subscription messages now also have the Highlighted flag
- The "highlight" order of ChannelPointsHighlight, FirstMessageHighlight, HypeChatHighlight, and WatchStreakHighlight are not the same as before. Maybe also Announcements if I get to those.
- There's no longer a way to keep a custom sound URL attached to a highlight while having the sound disabled. I intend to add a volume slider so you could achieve the same/something similar by setting the volume to 0
- This has removed the "Highlight inline whispers" setting under the general section, and is now controlled by the "Inline whispers" highlight being enabled or disabled.

NOTE: If a user happens to have duplicate built-in highlights, should we clean that up on launch?
