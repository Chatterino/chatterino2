# Changelog

## Unversioned

## 2.3.0

- Major: Added clip creation support. You can create clips with `/clip` command, `Alt+X` keybind or `Create a clip` option in split header's context menu. This requires a new authentication scope so re-authentication will be required to use it. (#2271, #2377, #2528)
- Major: Added "Channel Filters". See https://wiki.chatterino.com/Filters/ for how they work or how to configure them. (#1748, #2083, #2090, #2200, #2225)
- Major: Added Streamer Mode configuration (under `Settings -> General`), where you can select which features of Chatterino should behave differently when you are in Streamer Mode. (#2001, #2316, #2342, #2376)
- Major: Add `/settitle` and `/setgame` commands, originally made for Mm2PL/Dankerino. (#2534, #2609)
- Major: Color mentions to match the mentioned users. You can disable this by unchecking "Color @usernames" under `Settings -> General -> Advanced (misc.)`. (#1963, #2284, #2597)
- Major: Commands `/ignore` and `/unignore` have been renamed to `/block` and `/unblock` in order to keep consistency with Twitch's terms. (#2370)
- Major: Added support for bit emotes - the ones you unlock after cheering to streamer. (#2550)
- Minor: Added `/clearmessages` command - does what "Burger menu -> More -> Clear messages" does. (#2485)
- Minor: Added `/marker` command - similar to webchat, it creates a stream marker. (#2360)
- Minor: Added `/chatters` command showing chatter count. (#2344)
- Minor: Added a button to the split context menu to open the moderation view for a channel when the account selected has moderator permissions. (#2321)
- Minor: Made BetterTTV emote tooltips use authors' display name. (#2267)
- Minor: Added Ctrl + 1/2/3/... and Ctrl+9 shortcuts to Emote Popup (activated with Ctrl+E). They work exactly the same as shortcuts in main window. (#2263)
- Minor: Added reconnect link to the "You are banned" message. (#2266)
- Minor: Improved search popup window titles. (#2268)
- Minor: Made "#channel" in `/mentions` tab a clickable link which takes you to the channel that you were mentioned in. (#2220)
- Minor: Added a keyboard shortcut (Ctrl+F5) for "Reconnect" (#2215)
- Minor: Made `Try to find usernames without @ prefix` option still resolve usernames when special characters (commas, dots, (semi)colons, exclamation mark, question mark) are appended to them. (#2212)
- Minor: Made usercard update user's display name (#2160)
- Minor: Added placeholder text for message text input box. (#2143, #2149, #2264)
- Minor: Added support for FrankerFaceZ badges. (#2101, part of #1658)
- Minor: Added a navigation list to the settings and reordered them.
- Minor: Added a link to twitchemotes.com to context menu when right-clicking Twitch emotes. (#2214)
- Minor: Improved viewer list window.
- Minor: Added emote completion with `:` to the whispers channel (#2075)
- Minor: Made the current channels emotes appear at the top of the emote picker popup. (#2057)
- Minor: Added viewer list button to twitch channel header. (#1978)
- Minor: Added followage and subage information to usercard. (#2023)
- Minor: Added an option to only open channels specified in command line with `-c` parameter. You can also use `--help` to display short help message (#1940, #2368)
- Minor: Added customizable timeout buttons to the user info popup
- Minor: Deprecate loading of "v1" window layouts. If you haven't updated Chatterino in more than 2 years, there's a chance you will lose your window layout.
- Minor: User popup will now automatically display messages as they are received. (#1982, #2514)
- Minor: Changed the English in two rate-limited system messages (#1878)
- Minor: Added a setting to disable messages sent to /mentions split from making the tab highlight with the red marker (#1994)
- Minor: Added image for streamer mode in the user popup icon.
- Minor: Added vip and unvip buttons.
- Minor: Added settings for displaying where the last message was.
- Minor: Commands are now saved upon pressing Ok in the settings window
- Minor: Colorized nicknames now enabled by default
- Minor: Show channels live now enabled by default
- Minor: Bold usernames enabled by default
- Minor: Improve UX of the "Login expired!" message (#2029)
- Minor: PageUp and PageDown now scroll in the selected split and in the emote popup (#2070, #2081, #2410, #2607)
- Minor: Allow highlights to be excluded from `/mentions`. Excluded highlights will not trigger tab highlights either. (#1793, #2036)
- Minor: Flag all popup dialogs as actual dialogs so they get the relevant window manager hints (#1843, #2182, #2185, #2232, #2234)
- Minor: Don't show update button for nightly builds on macOS and Linux, this was already the case for Windows (#2163, #2164)
- Minor: Tab and split titles now use display/localized channel names (#2189)
- Minor: Add a setting to limit the amount of historical messages loaded from the Recent Messages API (#2250, #2252)
- Minor: Made username autocompletion truecase (#1199, #1883)
- Minor: Update the listing of top-level domains. (#2345)
- Minor: Properly respect RECONNECT messages from Twitch (#2347)
- Minor: Hide "Case-sensitive" column for user highlights. (#2404)
- Minor: Added human-readable formatting to remaining timeout duration. (#2398)
- Minor: Update emojis version to 13 (2020). (#1555)
- Minor: Remove EmojiOne 2 and 3 due to license restrictions. (#1555)
- Minor: Added `/streamlink` command. Usage: `/streamlink <channel>`. You can also use the command without arguments in any twitch channel to open it in streamlink. (#2443, #2495)
- Minor: Humanized all numbers visible to end-users. (#2488)
- Minor: Added a context menu to avatar in usercard. It opens on right-clicking the avatar in usercard. (#2517)
- Minor: Handle messages that users can share after unlocking a new bits badge. (#2611)
- Bugfix: Fix crash occurring when pressing Escape in the Color Picker Dialog (#1843)
- Bugfix: Fix bug where the "check user follow state" event could trigger a network request requesting the user to follow or unfollow a user. By itself its quite harmless as it just repeats to Twitch the same follow state we had, so no follows should have been lost by this but it meant there was a rogue network request that was fired that could cause a crash (#1906)
- Bugfix: /usercard command will now respect the "Automatically close user popup" setting (#1918)
- Bugfix: Handle symlinks properly when saving commands & settings (#1856, #1908)
- Bugfix: Starting Chatterino in a minimized state after an update will no longer cause a crash
- Bugfix: Modify the emote parsing to handle some edge-cases with dots and stuff. (#1704, #1714, #2490)
- Bugfix: Fixed timestamps being incorrect on some messages loaded from the recent-messages service on startup (#1286, #2020)
- Bugfix: Fixed timestamps missing on channel point redemption messages (#1943)
- Bugfix: Fixed tooltip didn't show in `EmotePopup` depending on the `Link preview` setting enabled or no (#2008)
- Bugfix: Fixed Stream thumbnail not updating after using the "Change channel" feature (#2074, #2080)
- Bugfix: Fixed previous link info not updating after `Link information` setting is enabled (#2054)
- Bugfix: Fix Tab key not working in the Ctrl+K Quick Switcher (#2065)
- Bugfix: Fix bug preventing moderator actions when viewing a user card from the search window (#1089)
- Bugfix: Fix `:` emote completion menu ignoring emote capitalization and inconsistent emote names. (#1962, #2543)
- Bugfix: Fix a bug that caused `Ignore page` to fall into an infinity loop with an empty pattern and regex enabled (#2125)
- Bugfix: Fix a crash caused by FrankerFaceZ responding with invalid emote links (#2191)
- Bugfix: Fix a freeze caused by ignored & replaced phrases followed by Twitch Emotes (#2231)
- Bugfix: Fix a crash bug that occurred when moving splits across windows and closing the "parent tab" (#2249, #2259)
- Bugfix: Fix a crash bug that occurred when the "Limit message height" setting was enabled and a message was being split up into multiple lines. IRC only. (#2329)
- Bugfix: Fix anonymous users being pinged by "username" justinfan64537 (#2156, #2352)
- Bugfix: Fixed hidden tooltips when always on top is active (#2384)
- Bugfix: Fix CLI arguments (`--help`, `--version`, `--channels`) not being respected (#2368, #2190)
- Bugfix: Fixed search field not being focused on popup open (#2540)
- Bugfix: Fix Twitch cheer emotes not displaying tooltips when hovered (#2434, #2503)
- Bugfix: Fix BTTV/FFZ channel emotes saying unknown error when no emotes found (#2542)
- Bugfix: Fix directory not opening when clicking "Open AppData Directory" setting button on macOS (#2531, #2537)
- Bugfix: Fix quickswitcher not respecting order of tabs when filtering (#2519, #2561)
- Bugfix: Fix GNOME not associating Chatterino's window with its desktop entry (#1863, #2587)
- Bugfix: Fix buffer overflow in emoji parsing. (#2602)
- Bugfix: Fix windows being brought back to life after the settings dialog was closed. (#1892, #2613)
- Dev: Updated minimum required Qt framework version to 5.12. (#2210)
- Dev: Migrated `Kraken::getUser` to Helix (#2260)
- Dev: Migrated `TwitchAccount::(un)followUser` from Kraken to Helix and moved it to `Helix::(un)followUser`. (#2306)
- Dev: Migrated `Kraken::getChannel` to Helix. (#2381)
- Dev: Migrated `TwitchAccount::(un)ignoreUser` to Helix and made `TwitchAccount::loadIgnores` use Helix call. (#2370)
- Dev: Build in CI with multiple Qt versions (#2349)
- Dev: Updated minimum required macOS version to 10.14 (#2386)
- Dev: Removed unused `humanize` library (#2422)

## 2.2.2

- Bugfix: Fix a potential crash related to channel point rewards (279a80b)

## 2.2.1

- Minor: Disable checking for updates on unsupported platforms (#1874)
- Bugfix: Fix bug preventing users from setting the highlight color of the second entry in the "User" highlights tab (#1898)

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
- Bugfix: Fix a crash when using middle click scroll on a chat window. (#1870)
- Settings open faster
- Dev: Fully remove Twitch Chatroom support
- Dev: Handle conversion of historical CLEARCHAT messages to NOTICE messages in Chatterino instead of relying on the Recent Messages API to handle it for us. (#1804)
