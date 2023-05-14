#pragma once

#include "controllers/hotkeys/HotkeyCategory.hpp"

#include <QString>

#include <map>
#include <vector>

inline const std::vector<std::pair<std::vector<QString>, QString>>
    HOTKEY_ARG_ON_OFF_TOGGLE = {
        {{}, "Toggle"},
        {{"on"}, "Set to on"},
        {{"off"}, "Set to off"},
};

inline const std::vector<std::pair<std::vector<QString>, QString>>
    HOTKEY_ARG_WITH_OR_WITHOUT_SELECTION = {
        {{"withoutSelection"}, "No"},
        {{"withSelection"}, "Yes"},
};

namespace chatterino {

// ActionDefinition is an action that can be performed with a hotkey
struct ActionDefinition {
    // displayName is the value that would be shown to a user when they edit or create a hotkey for an action
    QString displayName;

    // argumentDescription is a description of the arguments in a format of
    // "<required arg: description of possible values> [optional arg: possible
    // values]"
    QString argumentDescription = "";

    // minCountArguments is the minimum amount of arguments the action accepts
    // Example action: "Select Tab" in a popup window accepts 1 argument for which tab to select
    uint8_t minCountArguments = 0;

    // maxCountArguments is the maximum amount of arguments the action accepts
    uint8_t maxCountArguments = minCountArguments;

    // possibleArguments is empty or contains all possible argument values,
    // left string from the pair is the thing the action will see,
    // right one is what the user sees
    std::vector<std::pair<std::vector<QString>, QString>> possibleArguments =
        {};

    // When possibleArguments are present this should be a string like
    // "Direction:" which will be shown before the values from
    // possibleArguments in the UI. Otherwise, it should be empty.
    QString argumentsPrompt = "";
    // A more detailed description of what argumentsPrompt means
    QString argumentsPromptHover = "";
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
              .displayName = "Scroll",
              .argumentDescription = "<direction: up or down>",
              .minCountArguments = 1,
              .maxCountArguments = 1,
              .possibleArguments{
                  {{"up"}, "Up"},
                  {{"down"}, "Down"},
              },
              .argumentsPrompt = "Direction:",
          }},
         {"search", ActionDefinition{"Focus search box"}},
         {"execModeratorAction",
          ActionDefinition{
              "Usercard: execute moderation action",
              "<ban, unban or number of the timeout button to use>", 1}},
     }},
    {HotkeyCategory::Split,
     {
         {"changeChannel", ActionDefinition{"Change channel"}},
         {"clearMessages", ActionDefinition{"Clear messages"}},
         {"createClip", ActionDefinition{"Create a clip"}},
         {"delete", ActionDefinition{"Close"}},
         {"focus",
          ActionDefinition{
              .displayName = "Focus neighbouring split",
              .argumentDescription = "<direction: up, down, left or right>",
              .minCountArguments = 1,
              .maxCountArguments = 1,
              .possibleArguments{
                  {{"up"}, "Up"},
                  {{"down"}, "Down"},
                  {{"left"}, "Left"},
                  {{"right"}, "Right"},
              },
              .argumentsPrompt = "Direction:",
              .argumentsPromptHover =
                  "Which direction to look for a split to focus?",
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
              .displayName = "Reload emotes",
              .argumentDescription =
                  "[type: channel or subscriber; default: all emotes]",
              .minCountArguments = 0,
              .maxCountArguments = 1,
              .possibleArguments{
                  {{}, "All emotes"},
                  {{"channel"}, "Channel emotes only"},
                  {{"subscriber"}, "Subscriber emotes only"},
              },
              .argumentsPrompt = "Emote type:",
              .argumentsPromptHover = "Which emotes should Chatterino reload",
          }},
         {"runCommand",
          ActionDefinition{
              "Run a command",
              "<name of command>",
              1,
          }},
         {"scrollPage",
          ActionDefinition{
              .displayName = "Scroll",
              .argumentDescription = "<up or down>",
              .minCountArguments = 1,
              .maxCountArguments = 1,
              .possibleArguments{
                  {{"up"}, "Up"},
                  {{"down"}, "Down"},
              },
              .argumentsPrompt = "Direction:",
              .argumentsPromptHover =
                  "Which direction do you want to see more messages",
          }},
         {"scrollToBottom", ActionDefinition{"Scroll to the bottom"}},
         {"scrollToTop", ActionDefinition{"Scroll to the top"}},
         {"setChannelNotification",
          ActionDefinition{
              .displayName = "Set channel live notification",
              .argumentDescription = "[on or off. default: toggle]",
              .minCountArguments = 0,
              .maxCountArguments = 1,
              .possibleArguments = HOTKEY_ARG_ON_OFF_TOGGLE,
              .argumentsPrompt = "New value:",
              .argumentsPromptHover = "Should the channel live notification be "
                                      "enabled, disabled or toggled",
          }},
         {"setModerationMode",
          ActionDefinition{
              .displayName = "Set moderation mode",
              .argumentDescription = "[on or off. default: toggle]",
              .minCountArguments = 0,
              .maxCountArguments = 1,
              .possibleArguments = HOTKEY_ARG_ON_OFF_TOGGLE,
              .argumentsPrompt = "New value:",
              .argumentsPromptHover =
                  "Should the moderation mode be enabled, disabled or toggled",
          }},
         {"showSearch", ActionDefinition{"Search current channel"}},
         {"showGlobalSearch", ActionDefinition{"Search all channels"}},
         {"startWatching", ActionDefinition{"Start watching"}},
         {"debug", ActionDefinition{"Show debug popup"}},
     }},
    {HotkeyCategory::SplitInput,
     {
         {"clear", ActionDefinition{"Clear message"}},
         {"copy",
          ActionDefinition{
              .displayName = "Copy",
              .argumentDescription =
                  "<source of text: auto, split or splitInput>",
              .minCountArguments = 1,
              .possibleArguments{
                  {{"auto"}, "Automatic"},
                  {{"split"}, "Split"},
                  {{"splitInput"}, "Split Input"},
              },
              .argumentsPrompt = "Source of text:",
          }},
         {"cursorToStart",
          ActionDefinition{
              .displayName = "To start of message",
              .argumentDescription =
                  "<selection mode: withSelection or withoutSelection>",
              .minCountArguments = 1,
              .maxCountArguments = 1,
              .possibleArguments = HOTKEY_ARG_WITH_OR_WITHOUT_SELECTION,
              .argumentsPrompt = "Select text from cursor to start:",
              // XXX: write a hover for this that doesn't suck
          }},
         {"cursorToEnd",
          ActionDefinition{
              .displayName = "To end of message",
              .argumentDescription =
                  "<selection mode: withSelection or withoutSelection>",
              .minCountArguments = 1,
              .maxCountArguments = 1,
              .possibleArguments = HOTKEY_ARG_WITH_OR_WITHOUT_SELECTION,
              .argumentsPrompt = "Select text from cursor to end:",
              // XXX: write a hover for this that doesn't suck
          }},
         {"nextMessage", ActionDefinition{"Choose next sent message"}},
         {"openEmotesPopup", ActionDefinition{"Open emotes list"}},
         {"paste", ActionDefinition{"Paste"}},
         {"previousMessage",
          ActionDefinition{"Choose previously sent message"}},
         {"redo", ActionDefinition{"Redo"}},
         {"selectAll", ActionDefinition{"Select all"}},
         {"selectWord", ActionDefinition{"Select word"}},
         {"sendMessage",
          ActionDefinition{
              .displayName = "Send message",
              .argumentDescription =
                  "[keepInput to not clear the text after sending]",
              .minCountArguments = 0,
              .maxCountArguments = 1,
              .possibleArguments{
                  {{}, "Default behavior"},
                  {{"keepInput"}, "Keep message in input after sending it"},
              },
              .argumentsPrompt = "Behavior:",
          }},
         {"undo", ActionDefinition{"Undo"}},

     }},
    {HotkeyCategory::Window,
     {
#ifndef NDEBUG
         {"addCheerMessage", ActionDefinition{"Debug: Add cheer test message"}},
         {"addEmoteMessage", ActionDefinition{"Debug: Add emote test message"}},
         {"addLinkMessage",
          ActionDefinition{"Debug: Add test message with a link"}},
         {"addMiscMessage", ActionDefinition{"Debug: Add misc test message"}},
         {"addRewardMessage",
          ActionDefinition{"Debug: Add reward test message"}},
         {"addSubMessage", ActionDefinition{"Debug: Add sub test message"}},
#endif
         {"moveTab",
          ActionDefinition{
              "Move tab",
              "<where to move the tab: next, previous, or new index of tab>",
              1,
          }},
         {"newSplit", ActionDefinition{"Create a new split"}},
         {"newTab", ActionDefinition{"Create a new tab"}},
         {"openSettings", ActionDefinition{"Open settings"}},
         {"openTab",
          ActionDefinition{
              "Select tab",
              "<which tab to select: last, next, previous, or index>",
              1,
          }},
         {"openQuickSwitcher", ActionDefinition{"Open the quick switcher"}},
         {"popup",
          ActionDefinition{
              .displayName = "New popup",
              .argumentDescription = "<split or window>",
              .minCountArguments = 1,
              .maxCountArguments = 1,
              .possibleArguments{
                  {{"split"}, "Focused Split"},
                  {{"window"}, "Entire Tab"},
              },
              .argumentsPrompt = "What should be copied",
          }},
         {"quit", ActionDefinition{"Quit Chatterino"}},
         {"removeTab", ActionDefinition{"Remove current tab"}},
         {"reopenSplit", ActionDefinition{"Reopen closed split"}},
         {"setStreamerMode",
          ActionDefinition{
              .displayName = "Set streamer mode",
              .argumentDescription =
                  "[on, off, toggle, or auto. default: toggle]",
              .minCountArguments = 0,
              .maxCountArguments = 1,
              .possibleArguments =
                  {
                      {{}, "Toggle on/off"},
                      {{"on"}, "Set to on"},
                      {{"off"}, "Set to off"},
                      {{"auto"}, "Set to automatic"},
                  },
              .argumentsPrompt = "New value:",
              .argumentsPromptHover =
                  "Should streamer mode be enabled, disabled, toggled (on/off) "
                  "or set to auto",
          }},
         {"toggleLocalR9K", ActionDefinition{"Toggle local R9K"}},
         {"zoom",
          ActionDefinition{
              .displayName = "Zoom in/out",
              .argumentDescription = "Argument:",
              .minCountArguments = 1,
              .maxCountArguments = 1,
              .possibleArguments =
                  {
                      {{"in"}, "Zoom in"},
                      {{"out"}, "Zoom out"},
                      {{"reset"}, "Reset zoom"},
                  },
          }},
         {"setTabVisibility",
          ActionDefinition{
              .displayName = "Set tab visibility",
              .argumentDescription = "[on, off, or toggle. default: toggle]",
              .minCountArguments = 0,
              .maxCountArguments = 1,
              .possibleArguments = HOTKEY_ARG_ON_OFF_TOGGLE,
              .argumentsPrompt = "New value:",
              .argumentsPromptHover =
                  "Should the tabs be enabled, disabled or toggled.",
          }},
     }},
};

}  // namespace chatterino
