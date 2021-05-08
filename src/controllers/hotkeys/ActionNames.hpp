#include "HotkeyScope.hpp"

namespace chatterino {
struct HotkeyDefinition {
    QString displayName;
    uint8_t minCountArguments = 0;
    uint8_t maxCountArguments = minCountArguments;
};

const static std::map<HotkeyScope, std::map<QString, HotkeyDefinition>>
    actionNames{
        {HotkeyScope::EmotePopup,
         {
             {"openTab", HotkeyDefinition{"Select Tab", 1}},
             {"delete", HotkeyDefinition{"Close"}},
             {"scrollPage", HotkeyDefinition{"Scroll", 1}},
         }},
        {HotkeyScope::SelectChannelPopup,
         {
             {"accept", HotkeyDefinition{"Confirm"}},
             {"reject", HotkeyDefinition{"Cancel"}},
         }},
        {HotkeyScope::Settings,
         {
             {"search", HotkeyDefinition{"Focus search box"}},
         }},
        {HotkeyScope::Split,
         {
             {"delete", HotkeyDefinition{"Close"}},
             {"changeChannel", HotkeyDefinition{"Change channel"}},
             {"showSearch", HotkeyDefinition{"Search"}},
             {"reconnect", HotkeyDefinition{"Reconnect to chat"}},
             {"focus", HotkeyDefinition{"Focus neighbouring split", 1}},
             {"scrollToBottom", HotkeyDefinition{"Scroll to the bottom"}},
             {"scrollPage", HotkeyDefinition{"Scroll", 1}},
             {"pickFilters", HotkeyDefinition{"Pick filters"}},
             {"startWatching", HotkeyDefinition{"Start watching"}},
             {"openInBrowser", HotkeyDefinition{"Open channel in browser"}},
             {"openInStreamlink",
              HotkeyDefinition{"Open stream in streamlink"}},
             {"openInCustomPlayer",
              HotkeyDefinition{"Open stream in custom player"}},
             {"openModView", HotkeyDefinition{"Open mod view in browser"}},
             {"createClip", HotkeyDefinition{"Create a clip"}},
             {"reloadEmotes", HotkeyDefinition{"Reload emotes", 0, 1}},
             {"setModerationMode",
              HotkeyDefinition{"Set moderation mode", 0, 1}},
             {"openViewerList", HotkeyDefinition{"Open viewer list"}},
             {"clearMessages", HotkeyDefinition{"Clear messages"}},
             {"runCommand", HotkeyDefinition{"Run a command", 1}},
             {"setChannelNotification",
              HotkeyDefinition{"Set channel live notification", 0, 1}},
         }},
        {HotkeyScope::SplitInput,
         {
             // TODO
             {"jumpCursor", HotkeyDefinition{"Move cursor to start or end", 2}},
             {"openEmotesPopup", HotkeyDefinition{"Open emotes list"}},
             {"sendMessage", HotkeyDefinition{"Send message"}},

             // XXX(mm2pl): these two need better display names
             {"previousMessage",
              HotkeyDefinition{"Pick previously edited message"}},
             {"nextMessage", HotkeyDefinition{"Pick next edited message"}},

             {"undo", HotkeyDefinition{"Undo"}},
             {"redo", HotkeyDefinition{"Redo"}},
             {"copy", HotkeyDefinition{"Copy"}},
             {"paste", HotkeyDefinition{"Paste"}},
             {"clear", HotkeyDefinition{"Clear message"}},
             {"selectAll", HotkeyDefinition{"Select all"}},
         }},
        {HotkeyScope::UserCard,
         {
             {"delete", HotkeyDefinition{"Close"}},
         }},
        {HotkeyScope::Window,
         {
             {"openSettings", HotkeyDefinition{"Open settings"}},
             {"newSplit", HotkeyDefinition{"Create a new split"}},
             {"openTab", HotkeyDefinition{"Select tab"}},
             {"popup", HotkeyDefinition{"New popup"}},
             {"zoom", HotkeyDefinition{"Zoom in/out", 1}},
             {"newTab", HotkeyDefinition{"Create a new tab"}},
             {"removeTab", HotkeyDefinition{"Remove current tab"}},
             {"reopenSplit", HotkeyDefinition{"Reopen closed split"}},
             {"toggleLocalR9K", HotkeyDefinition{"Toggle local R9K"}},
             {"openQuickSwitcher", HotkeyDefinition{"Open the quick switcher"}},
             {"quit", HotkeyDefinition{"Quit Chatterino"}},
             {"moveTab", HotkeyDefinition{"Move tab", 1}},
             {"setStreamerMode", HotkeyDefinition{"Set streamer mode", 1}},
         }},
    };

}  // namespace chatterino
