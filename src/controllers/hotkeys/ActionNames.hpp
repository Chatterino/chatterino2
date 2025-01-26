#pragma once

#include "controllers/hotkeys/HotkeyCategory.hpp"

#include <QString>

#include <map>
#include <vector>

inline const std::vector<std::pair<QString, std::vector<QString>>>
    HOTKEY_ARG_ON_OFF_TOGGLE = {
        {"Toggle", {}},
        {"Set to on", {"on"}},
        {"Set to off", {"off"}},
};

inline const std::vector<std::pair<QString, std::vector<QString>>>
    HOTKEY_ARG_WITH_OR_WITHOUT_SELECTION = {
        {"No", {"withoutSelection"}},
        {"Yes", {"withSelection"}},
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
    // it is an ordered mapping from option name (what the user sees) to
    // arguments (what the action code will see).
    // As std::map<K, V> does not guarantee order this is a std::vector<...>
    std::vector<std::pair<QString, std::vector<QString>>> possibleArguments =
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
                  {"Up", {"up"}},
                  {"Down", {"down"}},
              },
              .argumentsPrompt = "Direction:",
          }},
         {"search", ActionDefinition{"Focus search box"}},
         {"execModeratorAction",
          ActionDefinition{
              "Usercard: execute moderation action",
              "<ban, unban or number of the timeout button to use>", 1}},
         {"pin", ActionDefinition{"Usercard, reply thread: pin window"}},
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
                  {"Up", {"up"}},
                  {"Down", {"down"}},
                  {"Left", {"left"}},
                  {"Right", {"right"}},
              },
              .argumentsPrompt = "Direction:",
              .argumentsPromptHover =
                  "Which direction to look for a split to focus?",
          }},
         {"openInBrowser", ActionDefinition{"Open channel in browser"}},
         {"openPlayerInBrowser",
          ActionDefinition{"Open stream in browser player"}},
         {"openInCustomPlayer",
          ActionDefinition{"Open stream in custom player"}},
         {"openInStreamlink", ActionDefinition{"Open stream in streamlink"}},
         {"openModView", ActionDefinition{"Open mod view in browser"}},
         {"openViewerList", ActionDefinition{"Open chatter list"}},
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
                  {"All emotes", {}},
                  {"Channel emotes only", {"channel"}},
                  {"Subscriber emotes only", {"subscriber"}},
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
                  {"Up", {"up"}},
                  {"Down", {"down"}},
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
         {"debug", ActionDefinition{"Show debug popup"}},
         {"popupOverlay", ActionDefinition{"New overlay popup"}},
         {"toggleOverlayInertia",
          ActionDefinition{
              .displayName = "Toggle overlay click-through",
              .argumentDescription = "<target popup: this or thisOrAll or all>",
              .minCountArguments = 1,
              .maxCountArguments = 1,
              .possibleArguments{
                  {"This", {"this"}},
                  {"All", {"all"}},
                  {"This or all", {"thisOrAll"}},
              },
              .argumentsPrompt = "Target popup:",
          }},
         {"setHighlightSounds",
          ActionDefinition{
              .displayName = "Set highlight sounds",
              .argumentDescription = "[on or off. default: toggle]",
              .minCountArguments = 0,
              .maxCountArguments = 1,
              .possibleArguments = HOTKEY_ARG_ON_OFF_TOGGLE,
              .argumentsPrompt = "New value:",
              .argumentsPromptHover =
                  "Should highlight sounds be enabled, disabled or toggled",
          }},
         {"openSubscriptionPage", ActionDefinition{"Open subscription page"}},
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
                  {"Automatic", {"auto"}},
                  {"Split", {"split"}},
                  {"Split Input", {"splitInput"}},
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
                  {"Default behavior", {}},
                  {"Keep message in input after sending it", {"keepInput"}},
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
              "<where to move the tab: next, previous, or new index of "
              "tab>",
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
                  {"Focused Split", {"split"}},
                  {"Entire Tab", {"window"}},
              },
              .argumentsPrompt = "Include:",
              .argumentsPromptHover =
                  "What should be included in the new popup",
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
                      {"Toggle on/off", {}},
                      {"Set to on", {"on"}},
                      {"Set to off", {"off"}},
                      {"Set to automatic", {"auto"}},
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
                      {"Zoom in", {"in"}},
                      {"Zoom out", {"out"}},
                      {"Reset zoom", {"reset"}},
                  },
              .argumentsPrompt = "Option:",
          }},
         {"setTabVisibility",
          ActionDefinition{
              .displayName = "Set tab visibility",
              .argumentDescription =
                  "[on, off, toggle, or liveOnly. default: toggle]",
              .minCountArguments = 0,
              .maxCountArguments = 1,
              .possibleArguments{
                  {"Toggle", {}},
                  {"Show all tabs", {"on"}},
                  {"Hide all tabs", {"off"}},
                  {"Only show live tabs", {"liveOnly"}},
              },
              .argumentsPrompt = "New value:",
              .argumentsPromptHover = "Should the tabs be enabled, disabled, "
                                      "toggled, or live-only.",
          }},
     }},
};

}  // namespace chatterino
