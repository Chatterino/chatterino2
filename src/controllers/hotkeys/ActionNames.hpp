#pragma once

#include "HotkeyCategory.hpp"

#include <QString>

#include <map>

namespace chatterino {

// ActionDefinition is an action that can be performed with a hotkey
struct ActionDefinition {
    // displayName is the value that would be shown to a user when they edit or create a hotkey for an action
    QString displayName;

    QString argumentDescription = "";

    // minCountArguments is the minimum amount of arguments the action accepts
    // Example action: "Select Tab" in a popup window accepts 1 argument for which tab to select
    uint8_t minCountArguments = 0;

    // maxCountArguments is the maximum amount of arguments the action accepts
    uint8_t maxCountArguments = minCountArguments;
};

using ActionDefinitionMap = std::map<QString, ActionDefinition>;

inline const std::map<HotkeyCategory, ActionDefinitionMap> actionNames{
    {HotkeyCategory::PopupWindow,
     {
         {"reject", ActionDefinition{"Confirmable popups: Cancel"}},
         {"accept", ActionDefinition{"Confirmable popups: Confirm"}},
         {"delete", ActionDefinition{"Close"}},
         {"openTab",
          ActionDefinition{
              "Select Tab",
              "<next, previous, or index of tab to select>",
              1,
          }},
         {"scrollPage",
          ActionDefinition{
              "Scroll",
              "<up or down>",
              1,
          }},
         {"search", ActionDefinition{"Focus search box"}},
     }},
    {HotkeyCategory::Split,
     {
         {"changeChannel", ActionDefinition{"Change channel"}},
         {"clearMessages", ActionDefinition{"Clear messages"}},
         {"createClip", ActionDefinition{"Create a clip"}},
         {"delete", ActionDefinition{"Close"}},
         {"focus",
          ActionDefinition{
              "Focus neighbouring split",
              "<up, down, left, or right>",
              1,
          }},
         {"openInBrowser", ActionDefinition{"Open channel in browser"}},
         {"openInCustomPlayer",
          ActionDefinition{"Open stream in custom player"}},
         {"openInStreamlink", ActionDefinition{"Open stream in streamlink"}},
         {"openModView", ActionDefinition{"Open mod view in browser"}},
         {"openViewerList", ActionDefinition{"Open viewer list"}},
         {"pickFilters", ActionDefinition{"Pick filters"}},
         {"reconnect", ActionDefinition{"Reconnect to chat"}},
         {"reloadEmotes",
          ActionDefinition{
              "Reload emotes",
              "[channel or subscriber]",
              0,
              1,
          }},
         {"runCommand",
          ActionDefinition{
              "Run a command",
              "<name of command>",
              1,
          }},
         {"scrollPage",
          ActionDefinition{
              "Scroll",
              "<up or down>",
              1,
          }},
         {"scrollToBottom", ActionDefinition{"Scroll to the bottom"}},
         {"setChannelNotification",
          ActionDefinition{
              "Set channel live notification",
              "[on or off. default: toggle]",
              0,
              1,
          }},
         {"setModerationMode",
          ActionDefinition{
              "Set moderation mode",
              "[on or off. default: toggle]",
              0,
              1,
          }},
         {"showSearch", ActionDefinition{"Search"}},
         {"startWatching", ActionDefinition{"Start watching"}},
         {"debug", ActionDefinition{"Show debug popup"}},
     }},
    {HotkeyCategory::SplitInput,
     {
         {"clear", ActionDefinition{"Clear message"}},
         {"copy",
          ActionDefinition{
              "Copy",
              "<source of text: split, splitInput or auto>",
              1,
          }},
         {"cursorToStart",
          ActionDefinition{
              "To start of message",
              "<withSelection or withoutSelection>",
              1,
          }},
         {"cursorToEnd",
          ActionDefinition{
              "To end of message",
              "<withSelection or withoutSelection>",
              1,
          }},
         {"nextMessage", ActionDefinition{"Choose next sent message"}},
         {"openEmotesPopup", ActionDefinition{"Open emotes list"}},
         {"paste", ActionDefinition{"Paste"}},
         {"previousMessage",
          ActionDefinition{"Choose previously sent message"}},
         {"redo", ActionDefinition{"Redo"}},
         {"selectAll", ActionDefinition{"Select all"}},
         {"sendMessage",
          ActionDefinition{
              "Send message",
              "[keepInput to not clear the text after sending]",
              0,
              1,
          }},
         {"undo", ActionDefinition{"Undo"}},

     }},
    {HotkeyCategory::Window,
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
         {"moveTab",
          ActionDefinition{
              "Move tab",
              "<next, previous, or new index of tab>",
              1,
          }},
         {"newSplit", ActionDefinition{"Create a new split"}},
         {"newTab", ActionDefinition{"Create a new tab"}},
         {"openSettings", ActionDefinition{"Open settings"}},
         {"openTab",
          ActionDefinition{
              "Select tab",
              "<last, next, previous, or index of tab to select>",
              1,
          }},
         {"openQuickSwitcher", ActionDefinition{"Open the quick switcher"}},
         {"popup",
          ActionDefinition{
              "New popup",
              "<split or window>",
              1,
          }},
         {"quit", ActionDefinition{"Quit Chatterino"}},
         {"removeTab", ActionDefinition{"Remove current tab"}},
         {"reopenSplit", ActionDefinition{"Reopen closed split"}},
         {"setStreamerMode",
          ActionDefinition{
              "Set streamer mode",
              "[on, off, toggle, or auto. default: toggle]",
              0,
              1,
          }},
         {"toggleLocalR9K", ActionDefinition{"Toggle local R9K"}},
         {"zoom",
          ActionDefinition{
              "Zoom in/out",
              "<in, out, or reset>",
              1,
          }},
         {"setTabVisibility",
          ActionDefinition{
              "Set tab visibility",
              "[on, off, or toggle. default: toggle]",
              0,
              1,
          }}}},
};

}  // namespace chatterino
