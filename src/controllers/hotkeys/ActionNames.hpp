#include "HotkeyScope.hpp"

namespace chatterino {
struct ActionDefinition {
    QString displayName;
    uint8_t minCountArguments = 0;
    uint8_t maxCountArguments = minCountArguments;
};

const std::map<HotkeyScope, std::map<QString, ActionDefinition>> actionNames{
    {HotkeyScope::PopupWindow,
     {
         {"reject", ActionDefinition{"Confirmable popups: Cancel"}},
         {"accept", ActionDefinition{"Confirmable popups: Confirm"}},
         {"delete", ActionDefinition{"Close"}},
         {"openTab", ActionDefinition{"Select Tab", 1}},
         {"scrollPage", ActionDefinition{"Scroll", 1}},
         {"search", ActionDefinition{"Focus search box"}},
     }},
    {HotkeyScope::Split,
     {
         {"changeChannel", ActionDefinition{"Change channel"}},
         {"clearMessages", ActionDefinition{"Clear messages"}},
         {"createClip", ActionDefinition{"Create a clip"}},
         {"delete", ActionDefinition{"Close"}},
         {"focus", ActionDefinition{"Focus neighbouring split", 1}},
         {"openInBrowser", ActionDefinition{"Open channel in browser"}},
         {"openInCustomPlayer",
          ActionDefinition{"Open stream in custom player"}},
         {"openInStreamlink", ActionDefinition{"Open stream in streamlink"}},
         {"openModView", ActionDefinition{"Open mod view in browser"}},
         {"openViewerList", ActionDefinition{"Open viewer list"}},
         {"pickFilters", ActionDefinition{"Pick filters"}},
         {"reconnect", ActionDefinition{"Reconnect to chat"}},
         {"reloadEmotes", ActionDefinition{"Reload emotes", 0, 1}},
         {"runCommand", ActionDefinition{"Run a command", 1}},
         {"scrollPage", ActionDefinition{"Scroll", 1}},
         {"scrollToBottom", ActionDefinition{"Scroll to the bottom"}},
         {"setChannelNotification",
          ActionDefinition{"Set channel live notification", 0, 1}},
         {"setModerationMode", ActionDefinition{"Set moderation mode", 0, 1}},
         {"showSearch", ActionDefinition{"Search"}},
         {"startWatching", ActionDefinition{"Start watching"}},
     }},
    {HotkeyScope::SplitInput,
     {
         // XXX(mm2pl): jumpCursor, nextMessage and previousMessage need better display names
         {"clear", ActionDefinition{"Clear message"}},
         {"copy", ActionDefinition{"Copy"}},
         {"jumpCursor", ActionDefinition{"Move cursor to start or end", 2}},
         {"nextMessage", ActionDefinition{"Pick next edited message"}},
         {"openEmotesPopup", ActionDefinition{"Open emotes list"}},
         {"paste", ActionDefinition{"Paste"}},
         {"previousMessage",
          ActionDefinition{"Pick previously edited message"}},
         {"redo", ActionDefinition{"Redo"}},
         {"selectAll", ActionDefinition{"Select all"}},
         {"sendMessage", ActionDefinition{"Send message"}},
         {"undo", ActionDefinition{"Undo"}},

     }},
    {HotkeyScope::Window,
     {
#ifdef C_DEBUG
         {"addCheerMessage", ActionDefinition{"Debug: Add cheer test message"}},
         {"addEmoteMessage", ActionDefinition{"Debug: Add emote test message"}},
         {"addLinkMessage",
          ActionDefinition{"Debug: Add test message with a link"}},
         {"addMiscMessage", ActionDefinition{"Debug: Add misc test message"}},
         {"addRewardMessage",
          ActionDefinition{"Debug: Add reward test message"}},
#endif
         {"moveTab", ActionDefinition{"Move tab", 1}},
         {"newSplit", ActionDefinition{"Create a new split"}},
         {"newTab", ActionDefinition{"Create a new tab"}},
         {"openSettings", ActionDefinition{"Open settings"}},
         {"openTab", ActionDefinition{"Select tab"}},
         {"openQuickSwitcher", ActionDefinition{"Open the quick switcher"}},
         {"popup", ActionDefinition{"New popup"}},
         {"quit", ActionDefinition{"Quit Chatterino"}},
         {"removeTab", ActionDefinition{"Remove current tab"}},
         {"reopenSplit", ActionDefinition{"Reopen closed split"}},
         {"setStreamerMode", ActionDefinition{"Set streamer mode", 1}},
         {"toggleLocalR9K", ActionDefinition{"Toggle local R9K"}},
         {"zoom", ActionDefinition{"Zoom in/out", 1}},
         {"setTabVisibility", ActionDefinition{"Set tab visibility", 1}}}},
};

}  // namespace chatterino
