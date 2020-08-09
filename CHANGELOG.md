# Changelog

## Unversioned
- Major: We now support image thumbnails coming from the link resolver. This feature is off by default and can be enabled in the settings with the "Show link thumbnail" setting. This feature also requires the "Show link info when hovering" setting to be enabled (#1664)
- Major: Added image upload functionality to i.nuuls.com with an ability to change upload destination. This works by dragging and dropping an image into a split, or pasting an image into the text edit field. (#1332, #1741)
- Minor: Add a switcher widget, similar to Discord. It can be opened by pressing Ctrl+K. (#1588)
- Minor: You can now open the Twitch User Card by middle-mouse clicking a username. (#1669)
- Minor: User Popup now also includes recent user messages (#1729)
- Minor: BetterTTV / FrankerFaceZ emote tooltips now also have emote authors' name (#1721)
- Minor: Emotes in the emote popup are now sorted in the same order as the tab completion (#1549)
- Minor: Removed "Online Logs" functionality as services are shut down (#1640)
- Bugfix: Fix preview on hover not working when Animated emotes options was disabled (#1546)
- Bugfix: FFZ custom mod badges no longer scale with the emote scale options (#1602)
- Bugfix: MacOS updater looked for non-existing fields, causing it to always fail the update check (#1642)
- Bugfix: Fixed message menu crashing if the message you right-clicked goes out of scope before you select an action (#1783) (#1787)
- Settings open faster
- Dev: Fully remove Twitch Chatroom support
