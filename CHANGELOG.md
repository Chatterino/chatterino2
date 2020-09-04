# Changelog

## Unversioned

- Minor: Added an option to only open channels specified in command line with `-c` parameter. You can also use `--help` to display short help message (#1940)
- Minor: Disable checking for updates on unsupported platforms (#1874)
- Bugfix: Fix bug preventing users from setting the highlight color of the second entry in the "User" highlights tab (#1898)
- Bugfix: /usercard command will now respect the "Automatically close user popup" setting (#1918)
- Bugfix: Handle symlinks properly when saving commands & settings (#1856, #1908)

## 2.2.0

- Major: We now support image thumbnails coming from the link resolver. This feature is off by default and can be enabled in the settings with the "Show link thumbnail" setting. This feature also requires the "Show link info when hovering" setting to be enabled (#1664)
- Major: Added image upload functionality to i.nuuls.com with an ability to change upload destination. This works by dragging and dropping an image into a split, or pasting an image into the text edit field. (#1332, #1741)
- Major: Added option to display tabs vertically. (#1815)
- Major: Support the highlighted messages redeemed with channel points on twitch.tv.
- Major: Added emote completion with `:`
- Minor: Added a "Streamer Mode" that hides user generated images while obs is open.
- Minor: Added extension support for Brave browser and Microsoft Edge. (#1862)
- Minor: Add a switcher widget, similar to Discord. It can be opened by pressing Ctrl+K. (#1588)
- Minor: Clicking on `Open in browser` in a whisper split will now open your whispers on twitch. (#1828)
- Minor: Clicking on @mentions will open the User Popup. (#1674)
- Minor: You can now open the Twitch User Card by middle-mouse clicking a username. (#1669)
- Minor: User Popup now also includes recent user messages (#1729)
- Minor: BetterTTV / FrankerFaceZ emote tooltips now also have emote authors' name (#1721)
- Minor: Emotes in the emote popup are now sorted in the same order as the tab completion (#1549)
- Minor: Removed "Online Logs" functionality as services are shut down (#1640)
- Minor: CTRL+F now selects the Find text input field in the Settings Dialog (#1806 #1811)
- Minor: CTRL+F now selects the search text input field in the Search Popup (#1812)
- Minor: Modify our word boundary logic in highlight phrase searching to accomodate non-regex phrases with "word-boundary-creating" characters like ! (#1885, #1890)
- Bugfix: Fixed not being able to open links in incognito with Microsoft Edge (Chromium) (#1875)
- Bugfix: Fix the incorrect `Open stream in browser` labelling in the whisper split (#1860)
- Bugfix: Fix preview on hover not working when Animated emotes options was disabled (#1546)
- Bugfix: FFZ custom mod badges no longer scale with the emote scale options (#1602)
- Bugfix: MacOS updater looked for non-existing fields, causing it to always fail the update check (#1642)
- Bugfix: Fixed message menu crashing if the message you right-clicked goes out of scope before you select an action (#1783) (#1787)
- Bugfix: Fixed alternate messages flickering in UserInfoPopup when clicking Refresh if there was an odd number of messages in there (#1789 #1810)
- Settings open faster
- Dev: Fully remove Twitch Chatroom support
- Dev: Handle conversion of historical CLEARCHAT messages to NOTICE messages in Chatterino instead of relying on the Recent Messages API to handle it for us. (#1804)
