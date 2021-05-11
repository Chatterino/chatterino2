#include "HotkeyScope.hpp"

namespace chatterino {
struct HotkeyDefinition {
    QString displayName;
    uint8_t minCountArguments = 0;
    uint8_t maxCountArguments = minCountArguments;
};

const std::map<HotkeyScope, std::map<QString, HotkeyDefinition>> actionNames{
    {HotkeyScope::PopupWindow,
     {
         {"reject", HotkeyDefinition{"Confirmable popups: Cancel"}},
         {"accept", HotkeyDefinition{"Confirmable popups: Confirm"}},
         {"delete", HotkeyDefinition{"Close"}},
         {"openTab", HotkeyDefinition{"Select Tab", 1}},
         {"scrollPage", HotkeyDefinition{"Scroll", 1}},
     }},
    {HotkeyScope::Settings,
     {
         {"search", HotkeyDefinition{"Focus search box"}},
     }},
    {HotkeyScope::Split,
     {
         {"changeChannel", HotkeyDefinition{"Change channel"}},
         {"clearMessages", HotkeyDefinition{"Clear messages"}},
         {"createClip", HotkeyDefinition{"Create a clip"}},
         {"delete", HotkeyDefinition{"Close"}},
         {"focus", HotkeyDefinition{"Focus neighbouring split", 1}},
         {"openInBrowser", HotkeyDefinition{"Open channel in browser"}},
         {"openInCustomPlayer",
          HotkeyDefinition{"Open stream in custom player"}},
         {"openInStreamlink", HotkeyDefinition{"Open stream in streamlink"}},
         {"openModView", HotkeyDefinition{"Open mod view in browser"}},
         {"openViewerList", HotkeyDefinition{"Open viewer list"}},
         {"pickFilters", HotkeyDefinition{"Pick filters"}},
         {"reconnect", HotkeyDefinition{"Reconnect to chat"}},
         {"reloadEmotes", HotkeyDefinition{"Reload emotes", 0, 1}},
         {"runCommand", HotkeyDefinition{"Run a command", 1}},
         {"scrollPage", HotkeyDefinition{"Scroll", 1}},
         {"scrollToBottom", HotkeyDefinition{"Scroll to the bottom"}},
         {"setChannelNotification",
          HotkeyDefinition{"Set channel live notification", 0, 1}},
         {"setModerationMode", HotkeyDefinition{"Set moderation mode", 0, 1}},
         {"showSearch", HotkeyDefinition{"Search"}},
         {"startWatching", HotkeyDefinition{"Start watching"}},
     }},
    {HotkeyScope::SplitInput,
     {
         // XXX(mm2pl): jumpCursor, nextMessage and previousMessage need better display names
         {"clear", HotkeyDefinition{"Clear message"}},
         {"copy", HotkeyDefinition{"Copy"}},
         {"jumpCursor", HotkeyDefinition{"Move cursor to start or end", 2}},
         {"nextMessage", HotkeyDefinition{"Pick next edited message"}},
         {"openEmotesPopup", HotkeyDefinition{"Open emotes list"}},
         {"paste", HotkeyDefinition{"Paste"}},
         {"previousMessage",
          HotkeyDefinition{"Pick previously edited message"}},
         {"redo", HotkeyDefinition{"Redo"}},
         {"selectAll", HotkeyDefinition{"Select all"}},
         {"sendMessage", HotkeyDefinition{"Send message"}},
         {"undo", HotkeyDefinition{"Undo"}},

     }},
    {HotkeyScope::Window,
     {
#ifdef C_DEBUG
         {"addCheerMessage", HotkeyDefinition{"Debug: Add cheer test message"}},
         {"addEmoteMessage", HotkeyDefinition{"Debug: Add emote test message"}},
         {"addLinkMessage",
          HotkeyDefinition{"Debug: Add test message with a link"}},
         {"addMiscMessage", HotkeyDefinition{"Debug: Add misc test message"}},
         {"addRewardMessage",
          HotkeyDefinition{"Debug: Add reward test message"}},
#endif
         {"moveTab", HotkeyDefinition{"Move tab", 1}},
         {"newSplit", HotkeyDefinition{"Create a new split"}},
         {"newTab", HotkeyDefinition{"Create a new tab"}},
         {"openSettings", HotkeyDefinition{"Open settings"}},
         {"openTab", HotkeyDefinition{"Select tab"}},
         {"openQuickSwitcher", HotkeyDefinition{"Open the quick switcher"}},
         {"popup", HotkeyDefinition{"New popup"}},
         {"quit", HotkeyDefinition{"Quit Chatterino"}},
         {"removeTab", HotkeyDefinition{"Remove current tab"}},
         {"reopenSplit", HotkeyDefinition{"Reopen closed split"}},
         {"setStreamerMode", HotkeyDefinition{"Set streamer mode", 1}},
         {"toggleLocalR9K", HotkeyDefinition{"Toggle local R9K"}},
         {"zoom", HotkeyDefinition{"Zoom in/out", 1}},
     }},
};

}  // namespace chatterino
