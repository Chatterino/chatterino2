set(LIBRARY_PROJECT "${PROJECT_NAME}-lib")
set(VERSION_PROJECT "${LIBRARY_PROJECT}-version")
set(EXECUTABLE_PROJECT "${PROJECT_NAME}")
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)
add_compile_definitions(QT_WARN_DEPRECATED_UP_TO=0x050F00)
add_compile_definitions(QT_NO_KEYWORDS)

# registers the native messageing host
option(CHATTERINO_DEBUG_NATIVE_MESSAGES "Debug native messages" OFF)
option(CHATTERINO_STATIC_QT_BUILD "Static link Qt" OFF)

set(SOURCE_FILES
        Application.cpp
        Application.hpp
        BrowserExtension.cpp
        BrowserExtension.hpp
        RunGui.cpp
        RunGui.hpp

        common/Args.cpp
        common/Args.hpp
        common/Channel.cpp
        common/Channel.hpp
        common/ChannelChatters.cpp
        common/ChannelChatters.hpp
        common/ChatterinoSetting.cpp
        common/ChatterinoSetting.hpp
        common/ChatterSet.cpp
        common/ChatterSet.hpp
        common/Credentials.cpp
        common/Credentials.hpp
        common/Env.cpp
        common/Env.hpp
        common/LinkParser.cpp
        common/LinkParser.hpp
        common/Literals.hpp
        common/Modes.cpp
        common/Modes.hpp
        common/QLogging.cpp
        common/QLogging.hpp
        common/WindowDescriptors.cpp
        common/WindowDescriptors.hpp

        common/enums/MessageContext.hpp
        common/enums/MessageOverflow.hpp

        common/network/NetworkCommon.cpp
        common/network/NetworkCommon.hpp
        common/network/NetworkManager.cpp
        common/network/NetworkManager.hpp
        common/network/NetworkPrivate.cpp
        common/network/NetworkPrivate.hpp
        common/network/NetworkRequest.cpp
        common/network/NetworkRequest.hpp
        common/network/NetworkResult.cpp
        common/network/NetworkResult.hpp
        common/network/NetworkTask.cpp
        common/network/NetworkTask.hpp

        controllers/accounts/Account.cpp
        controllers/accounts/Account.hpp
        controllers/accounts/AccountController.cpp
        controllers/accounts/AccountController.hpp
        controllers/accounts/AccountModel.cpp
        controllers/accounts/AccountModel.hpp

        controllers/commands/builtin/chatterino/Debugging.cpp
        controllers/commands/builtin/chatterino/Debugging.hpp
        controllers/commands/builtin/Misc.cpp
        controllers/commands/builtin/Misc.hpp
        controllers/commands/builtin/twitch/AddModerator.cpp
        controllers/commands/builtin/twitch/AddModerator.hpp
        controllers/commands/builtin/twitch/AddVIP.cpp
        controllers/commands/builtin/twitch/AddVIP.hpp
        controllers/commands/builtin/twitch/Announce.cpp
        controllers/commands/builtin/twitch/Announce.hpp
        controllers/commands/builtin/twitch/Ban.cpp
        controllers/commands/builtin/twitch/Ban.hpp
        controllers/commands/builtin/twitch/Block.cpp
        controllers/commands/builtin/twitch/Block.hpp
        controllers/commands/builtin/twitch/ChatSettings.cpp
        controllers/commands/builtin/twitch/ChatSettings.hpp
        controllers/commands/builtin/twitch/Chatters.cpp
        controllers/commands/builtin/twitch/Chatters.hpp
        controllers/commands/builtin/twitch/DeleteMessages.cpp
        controllers/commands/builtin/twitch/DeleteMessages.hpp
        controllers/commands/builtin/twitch/GetModerators.cpp
        controllers/commands/builtin/twitch/GetModerators.hpp
        controllers/commands/builtin/twitch/GetVIPs.cpp
        controllers/commands/builtin/twitch/GetVIPs.hpp
        controllers/commands/builtin/twitch/Raid.cpp
        controllers/commands/builtin/twitch/Raid.hpp
        controllers/commands/builtin/twitch/RemoveModerator.cpp
        controllers/commands/builtin/twitch/RemoveModerator.hpp
        controllers/commands/builtin/twitch/RemoveVIP.cpp
        controllers/commands/builtin/twitch/RemoveVIP.hpp
        controllers/commands/builtin/twitch/SendReply.cpp
        controllers/commands/builtin/twitch/SendReply.hpp
        controllers/commands/builtin/twitch/SendWhisper.cpp
        controllers/commands/builtin/twitch/SendWhisper.hpp
        controllers/commands/builtin/twitch/ShieldMode.cpp
        controllers/commands/builtin/twitch/ShieldMode.hpp
        controllers/commands/builtin/twitch/Shoutout.cpp
        controllers/commands/builtin/twitch/Shoutout.hpp
        controllers/commands/builtin/twitch/StartCommercial.cpp
        controllers/commands/builtin/twitch/StartCommercial.hpp
        controllers/commands/builtin/twitch/Unban.cpp
        controllers/commands/builtin/twitch/Unban.hpp
        controllers/commands/builtin/twitch/UpdateChannel.cpp
        controllers/commands/builtin/twitch/UpdateChannel.hpp
        controllers/commands/builtin/twitch/UpdateColor.cpp
        controllers/commands/builtin/twitch/UpdateColor.hpp
        controllers/commands/builtin/twitch/Warn.cpp
        controllers/commands/builtin/twitch/Warn.hpp
        controllers/commands/common/ChannelAction.cpp
        controllers/commands/common/ChannelAction.hpp
        controllers/commands/CommandContext.hpp
        controllers/commands/CommandController.cpp
        controllers/commands/CommandController.hpp
        controllers/commands/Command.cpp
        controllers/commands/Command.hpp
        controllers/commands/CommandModel.cpp
        controllers/commands/CommandModel.hpp

        controllers/completion/CompletionModel.cpp
        controllers/completion/CompletionModel.hpp
        controllers/completion/sources/Source.hpp
        controllers/completion/sources/CommandSource.cpp
        controllers/completion/sources/CommandSource.hpp
        controllers/completion/sources/EmoteSource.cpp
        controllers/completion/sources/EmoteSource.hpp
        controllers/completion/sources/Helpers.hpp
        controllers/completion/sources/UnifiedSource.cpp
        controllers/completion/sources/UnifiedSource.hpp
        controllers/completion/sources/UserSource.cpp
        controllers/completion/sources/UserSource.hpp
        controllers/completion/strategies/ClassicEmoteStrategy.cpp
        controllers/completion/strategies/ClassicEmoteStrategy.hpp
        controllers/completion/strategies/ClassicUserStrategy.cpp
        controllers/completion/strategies/ClassicUserStrategy.hpp
        controllers/completion/strategies/CommandStrategy.cpp
        controllers/completion/strategies/CommandStrategy.hpp
        controllers/completion/strategies/SmartEmoteStrategy.cpp
        controllers/completion/strategies/SmartEmoteStrategy.cpp
        controllers/completion/strategies/Strategy.hpp
        controllers/completion/TabCompletionModel.cpp
        controllers/completion/TabCompletionModel.hpp

        controllers/filters/FilterModel.cpp
        controllers/filters/FilterModel.hpp
        controllers/filters/FilterRecord.cpp
        controllers/filters/FilterRecord.hpp
        controllers/filters/FilterSet.cpp
        controllers/filters/FilterSet.hpp
        controllers/filters/lang/expressions/Expression.cpp
        controllers/filters/lang/expressions/Expression.hpp
        controllers/filters/lang/expressions/BinaryOperation.cpp
        controllers/filters/lang/expressions/BinaryOperation.hpp
        controllers/filters/lang/expressions/ListExpression.cpp
        controllers/filters/lang/expressions/ListExpression.hpp
        controllers/filters/lang/expressions/RegexExpression.cpp
        controllers/filters/lang/expressions/RegexExpression.hpp
        controllers/filters/lang/expressions/UnaryOperation.hpp
        controllers/filters/lang/expressions/UnaryOperation.cpp
        controllers/filters/lang/expressions/ValueExpression.cpp
        controllers/filters/lang/expressions/ValueExpression.hpp
        controllers/filters/lang/Filter.cpp
        controllers/filters/lang/Filter.hpp
        controllers/filters/lang/FilterParser.cpp
        controllers/filters/lang/FilterParser.hpp
        controllers/filters/lang/Tokenizer.cpp
        controllers/filters/lang/Tokenizer.hpp
        controllers/filters/lang/Types.cpp
        controllers/filters/lang/Types.hpp

        controllers/highlights/BadgeHighlightModel.cpp
        controllers/highlights/BadgeHighlightModel.hpp
        controllers/highlights/HighlightBadge.cpp
        controllers/highlights/HighlightBadge.hpp
        controllers/highlights/HighlightBlacklistModel.cpp
        controllers/highlights/HighlightBlacklistModel.hpp
        controllers/highlights/HighlightController.cpp
        controllers/highlights/HighlightController.hpp
        controllers/highlights/HighlightModel.cpp
        controllers/highlights/HighlightModel.hpp
        controllers/highlights/HighlightPhrase.cpp
        controllers/highlights/HighlightPhrase.hpp
        controllers/highlights/UserHighlightModel.cpp
        controllers/highlights/UserHighlightModel.hpp

        controllers/hotkeys/ActionNames.hpp
        controllers/hotkeys/Hotkey.cpp
        controllers/hotkeys/Hotkey.hpp
        controllers/hotkeys/HotkeyCategory.hpp
        controllers/hotkeys/HotkeyController.cpp
        controllers/hotkeys/HotkeyController.hpp
        controllers/hotkeys/HotkeyHelpers.cpp
        controllers/hotkeys/HotkeyHelpers.hpp
        controllers/hotkeys/HotkeyModel.cpp
        controllers/hotkeys/HotkeyModel.hpp

        controllers/ignores/IgnoreController.cpp
        controllers/ignores/IgnoreController.hpp
        controllers/ignores/IgnoreModel.cpp
        controllers/ignores/IgnoreModel.hpp
        controllers/ignores/IgnorePhrase.cpp
        controllers/ignores/IgnorePhrase.hpp

        controllers/moderationactions/ModerationAction.cpp
        controllers/moderationactions/ModerationAction.hpp
        controllers/moderationactions/ModerationActionModel.cpp
        controllers/moderationactions/ModerationActionModel.hpp

        controllers/logging/ChannelLog.cpp
        controllers/logging/ChannelLog.hpp
        controllers/logging/ChannelLoggingModel.cpp
        controllers/logging/ChannelLoggingModel.hpp

        controllers/nicknames/NicknamesModel.cpp
        controllers/nicknames/NicknamesModel.hpp
        controllers/nicknames/Nickname.hpp

        controllers/notifications/NotificationController.cpp
        controllers/notifications/NotificationController.hpp
        controllers/notifications/NotificationModel.cpp
        controllers/notifications/NotificationModel.hpp

        controllers/pings/MutedChannelModel.cpp
        controllers/pings/MutedChannelModel.hpp


        controllers/plugins/api/ChannelRef.cpp
        controllers/plugins/api/ChannelRef.hpp
        controllers/plugins/api/EventType.hpp
        controllers/plugins/api/HTTPRequest.cpp
        controllers/plugins/api/HTTPRequest.hpp
        controllers/plugins/api/HTTPResponse.cpp
        controllers/plugins/api/HTTPResponse.hpp
        controllers/plugins/api/IOWrapper.cpp
        controllers/plugins/api/IOWrapper.hpp
        controllers/plugins/LuaAPI.cpp
        controllers/plugins/LuaAPI.hpp
        controllers/plugins/LuaUtilities.cpp
        controllers/plugins/LuaUtilities.hpp
        controllers/plugins/PluginController.cpp
        controllers/plugins/PluginController.hpp
        controllers/plugins/Plugin.cpp
        controllers/plugins/Plugin.hpp
        controllers/plugins/PluginPermission.cpp
        controllers/plugins/PluginPermission.hpp
        controllers/plugins/SolTypes.cpp
        controllers/plugins/SolTypes.hpp

        controllers/sound/ISoundController.hpp
        controllers/sound/MiniaudioBackend.cpp
        controllers/sound/MiniaudioBackend.hpp
        controllers/sound/NullBackend.cpp
        controllers/sound/NullBackend.hpp

        controllers/twitch/LiveController.cpp
        controllers/twitch/LiveController.hpp

        controllers/userdata/UserDataController.cpp
        controllers/userdata/UserDataController.hpp
        controllers/userdata/UserData.hpp

        debug/Benchmark.cpp
        debug/Benchmark.hpp

        messages/Emote.cpp
        messages/Emote.hpp
        messages/Image.cpp
        messages/Image.hpp
        messages/ImageSet.cpp
        messages/ImageSet.hpp
        messages/Link.cpp
        messages/Link.hpp
        messages/Message.cpp
        messages/Message.hpp
        messages/MessageBuilder.cpp
        messages/MessageBuilder.hpp
        messages/MessageColor.cpp
        messages/MessageColor.hpp
        messages/MessageElement.cpp
        messages/MessageElement.hpp
        messages/MessageFlag.hpp
        messages/MessageSimilarity.cpp
        messages/MessageSimilarity.hpp
        messages/MessageSink.hpp
        messages/MessageThread.cpp
        messages/MessageThread.hpp

        messages/layouts/MessageLayout.cpp
        messages/layouts/MessageLayout.hpp
        messages/layouts/MessageLayoutContainer.cpp
        messages/layouts/MessageLayoutContainer.hpp
        messages/layouts/MessageLayoutContext.cpp
        messages/layouts/MessageLayoutContext.hpp
        messages/layouts/MessageLayoutElement.cpp
        messages/layouts/MessageLayoutElement.hpp
        messages/search/AuthorPredicate.cpp
        messages/search/AuthorPredicate.hpp
        messages/search/BadgePredicate.cpp
        messages/search/BadgePredicate.hpp
        messages/search/ChannelPredicate.cpp
        messages/search/ChannelPredicate.hpp
        messages/search/LinkPredicate.cpp
        messages/search/LinkPredicate.hpp
        messages/search/MessageFlagsPredicate.cpp
        messages/search/MessageFlagsPredicate.hpp
        messages/search/RegexPredicate.cpp
        messages/search/RegexPredicate.hpp
        messages/search/SubstringPredicate.cpp
        messages/search/SubstringPredicate.hpp
        messages/search/SubtierPredicate.cpp
        messages/search/SubtierPredicate.hpp

        providers/IvrApi.cpp
        providers/IvrApi.hpp
        providers/NetworkConfigurationProvider.cpp
        providers/NetworkConfigurationProvider.hpp

        providers/bttv/BttvEmotes.cpp
        providers/bttv/BttvEmotes.hpp
        providers/bttv/BttvLiveUpdates.cpp
        providers/bttv/BttvLiveUpdates.hpp

        providers/bttv/liveupdates/BttvLiveUpdateMessages.cpp
        providers/bttv/liveupdates/BttvLiveUpdateMessages.hpp
        providers/bttv/liveupdates/BttvLiveUpdateSubscription.cpp
        providers/bttv/liveupdates/BttvLiveUpdateSubscription.hpp

        providers/chatterino/ChatterinoBadges.cpp
        providers/chatterino/ChatterinoBadges.hpp

        providers/colors/ColorProvider.cpp
        providers/colors/ColorProvider.hpp

        providers/emoji/Emojis.cpp
        providers/emoji/Emojis.hpp

        providers/ffz/FfzBadges.cpp
        providers/ffz/FfzBadges.hpp
        providers/ffz/FfzEmotes.cpp
        providers/ffz/FfzEmotes.hpp
        providers/ffz/FfzUtil.cpp
        providers/ffz/FfzUtil.hpp

        providers/irc/IrcConnection2.cpp
        providers/irc/IrcConnection2.hpp

        providers/links/LinkInfo.cpp
        providers/links/LinkInfo.hpp
        providers/links/LinkResolver.cpp
        providers/links/LinkResolver.hpp

        providers/liveupdates/BasicPubSubClient.hpp
        providers/liveupdates/BasicPubSubManager.hpp
        providers/liveupdates/BasicPubSubWebsocket.hpp

        providers/pronouns/Pronouns.cpp
        providers/pronouns/Pronouns.hpp
        providers/pronouns/UserPronouns.cpp
        providers/pronouns/UserPronouns.hpp
        providers/pronouns/alejo/PronounsAlejoApi.cpp
        providers/pronouns/alejo/PronounsAlejoApi.hpp

        providers/recentmessages/Api.cpp
        providers/recentmessages/Api.hpp
        providers/recentmessages/Impl.cpp
        providers/recentmessages/Impl.hpp

        providers/seventv/SeventvAPI.cpp
        providers/seventv/SeventvAPI.hpp
        providers/seventv/SeventvBadges.cpp
        providers/seventv/SeventvBadges.hpp
        providers/seventv/SeventvCosmetics.hpp
        providers/seventv/SeventvEmotes.cpp
        providers/seventv/SeventvEmotes.hpp
        providers/seventv/SeventvEventAPI.cpp
        providers/seventv/SeventvEventAPI.hpp

        providers/seventv/eventapi/Client.cpp
        providers/seventv/eventapi/Client.hpp
        providers/seventv/eventapi/Dispatch.cpp
        providers/seventv/eventapi/Dispatch.hpp
        providers/seventv/eventapi/Message.cpp
        providers/seventv/eventapi/Message.hpp
        providers/seventv/eventapi/Subscription.cpp
        providers/seventv/eventapi/Subscription.hpp

        providers/twitch/ChannelPointReward.cpp
        providers/twitch/ChannelPointReward.hpp
        providers/twitch/IrcMessageHandler.cpp
        providers/twitch/IrcMessageHandler.hpp
        providers/twitch/PubSubActions.cpp
        providers/twitch/PubSubActions.hpp
        providers/twitch/PubSubClient.cpp
        providers/twitch/PubSubClient.hpp
        providers/twitch/PubSubClientOptions.hpp
        providers/twitch/PubSubHelpers.hpp
        providers/twitch/PubSubManager.cpp
        providers/twitch/PubSubManager.hpp
        providers/twitch/PubSubMessages.hpp
        providers/twitch/PubSubWebsocket.hpp
        providers/twitch/TwitchAccount.cpp
        providers/twitch/TwitchAccount.hpp
        providers/twitch/TwitchAccountManager.cpp
        providers/twitch/TwitchAccountManager.hpp
        providers/twitch/TwitchBadge.cpp
        providers/twitch/TwitchBadge.hpp
        providers/twitch/TwitchBadges.cpp
        providers/twitch/TwitchBadges.hpp
        providers/twitch/TwitchChannel.cpp
        providers/twitch/TwitchChannel.hpp
        providers/twitch/TwitchCommon.cpp
        providers/twitch/TwitchCommon.hpp
        providers/twitch/TwitchEmotes.cpp
        providers/twitch/TwitchEmotes.hpp
        providers/twitch/TwitchHelpers.cpp
        providers/twitch/TwitchHelpers.hpp
        providers/twitch/TwitchIrc.cpp
        providers/twitch/TwitchIrc.hpp
        providers/twitch/TwitchIrcServer.cpp
        providers/twitch/TwitchIrcServer.hpp
        providers/twitch/TwitchUser.cpp
        providers/twitch/TwitchUser.hpp
        providers/twitch/TwitchUsers.cpp
        providers/twitch/TwitchUsers.hpp

        providers/twitch/eventsub/Connection.cpp
        providers/twitch/eventsub/Connection.hpp
        providers/twitch/eventsub/Controller.cpp
        providers/twitch/eventsub/Controller.hpp
        providers/twitch/eventsub/MessageBuilder.cpp
        providers/twitch/eventsub/MessageBuilder.hpp
        providers/twitch/eventsub/MessageHandlers.cpp
        providers/twitch/eventsub/MessageHandlers.hpp
        providers/twitch/eventsub/SubscriptionHandle.cpp
        providers/twitch/eventsub/SubscriptionHandle.hpp
        providers/twitch/eventsub/SubscriptionRequest.cpp
        providers/twitch/eventsub/SubscriptionRequest.hpp

        providers/twitch/pubsubmessages/AutoMod.cpp
        providers/twitch/pubsubmessages/AutoMod.hpp
        providers/twitch/pubsubmessages/Base.cpp
        providers/twitch/pubsubmessages/Base.hpp
        providers/twitch/pubsubmessages/ChannelPoints.cpp
        providers/twitch/pubsubmessages/ChannelPoints.hpp
        providers/twitch/pubsubmessages/ChatModeratorAction.cpp
        providers/twitch/pubsubmessages/ChatModeratorAction.hpp
        providers/twitch/pubsubmessages/Listen.cpp
        providers/twitch/pubsubmessages/Listen.hpp
        providers/twitch/pubsubmessages/LowTrustUsers.cpp
        providers/twitch/pubsubmessages/LowTrustUsers.hpp
        providers/twitch/pubsubmessages/Message.hpp
        providers/twitch/pubsubmessages/Unlisten.cpp
        providers/twitch/pubsubmessages/Unlisten.hpp

        providers/twitch/api/Helix.cpp
        providers/twitch/api/Helix.hpp

        singletons/CrashHandler.cpp
        singletons/CrashHandler.hpp
        singletons/Emotes.cpp
        singletons/Emotes.hpp
        singletons/Fonts.cpp
        singletons/Fonts.hpp
        singletons/ImageUploader.cpp
        singletons/ImageUploader.hpp
        singletons/Logging.cpp
        singletons/Logging.hpp
        singletons/NativeMessaging.cpp
        singletons/NativeMessaging.hpp
        singletons/Paths.cpp
        singletons/Paths.hpp
        singletons/Resources.cpp
        singletons/Resources.hpp
        singletons/Settings.cpp
        singletons/Settings.hpp
        singletons/StreamerMode.cpp
        singletons/StreamerMode.hpp
        singletons/Theme.cpp
        singletons/Theme.hpp
        singletons/Toasts.cpp
        singletons/Toasts.hpp
        singletons/Updates.cpp
        singletons/Updates.hpp
        singletons/WindowManager.cpp
        singletons/WindowManager.hpp

        singletons/helper/GifTimer.cpp
        singletons/helper/GifTimer.hpp
        singletons/helper/LoggingChannel.cpp
        singletons/helper/LoggingChannel.hpp

        util/AbandonObject.hpp
        util/AttachToConsole.cpp
        util/AttachToConsole.hpp
        util/CancellationToken.hpp
        util/ChannelHelpers.hpp
        util/Clipboard.cpp
        util/Clipboard.hpp
        util/CustomPlayer.cpp
        util/CustomPlayer.hpp
        util/DebugCount.cpp
        util/DebugCount.hpp
        util/DisplayBadge.cpp
        util/DisplayBadge.hpp
        util/Expected.hpp
        util/FilesystemHelpers.hpp
        util/FormatTime.cpp
        util/FormatTime.hpp
        util/FunctionEventFilter.cpp
        util/FunctionEventFilter.hpp
        util/FuzzyConvert.cpp
        util/FuzzyConvert.hpp
        util/Helpers.cpp
        util/Helpers.hpp
        util/IncognitoBrowser.cpp
        util/IncognitoBrowser.hpp
        util/InitUpdateButton.cpp
        util/InitUpdateButton.hpp
        util/IpcQueue.cpp
        util/IpcQueue.hpp
        util/IrcHelpers.cpp
        util/IrcHelpers.hpp
        util/LayoutHelper.cpp
        util/LayoutHelper.hpp
        util/LoadPixmap.cpp
        util/LoadPixmap.hpp
        util/OnceFlag.cpp
        util/OnceFlag.hpp
        util/RapidjsonHelpers.cpp
        util/RapidjsonHelpers.hpp
        util/RatelimitBucket.cpp
        util/RatelimitBucket.hpp
        util/RenameThread.cpp
        util/RenameThread.hpp
        util/SampleData.cpp
        util/SampleData.hpp
        util/SharedPtrElementLess.hpp
        util/SignalListener.hpp
        util/StreamLink.cpp
        util/StreamLink.hpp
        util/ThreadGuard.hpp
        util/Twitch.cpp
        util/Twitch.hpp
        util/TypeName.hpp
        util/UnixSignalHandler.cpp
        util/UnixSignalHandler.hpp
        util/Variant.hpp
        util/VectorMessageSink.cpp
        util/VectorMessageSink.hpp
        util/WidgetHelpers.cpp
        util/WidgetHelpers.hpp
        util/WindowsHelper.cpp
        util/WindowsHelper.hpp
        util/XDGDesktopFile.cpp
        util/XDGDesktopFile.hpp
        util/XDGDirectory.cpp
        util/XDGDirectory.hpp
        util/XDGHelper.cpp
        util/XDGHelper.hpp

        util/serialize/Container.hpp

        widgets/AccountSwitchPopup.cpp
        widgets/AccountSwitchPopup.hpp
        widgets/AccountSwitchWidget.cpp
        widgets/AccountSwitchWidget.hpp
        widgets/AttachedWindow.cpp
        widgets/AttachedWindow.hpp
        widgets/BasePopup.cpp
        widgets/BasePopup.hpp
        widgets/BaseWidget.cpp
        widgets/BaseWidget.hpp
        widgets/BaseWindow.cpp
        widgets/BaseWindow.hpp
        widgets/DraggablePopup.cpp
        widgets/DraggablePopup.hpp
        widgets/FramelessEmbedWindow.cpp
        widgets/FramelessEmbedWindow.hpp
        widgets/Label.cpp
        widgets/Label.hpp
        widgets/Notebook.cpp
        widgets/Notebook.hpp
        widgets/OverlayWindow.cpp
        widgets/OverlayWindow.hpp
        widgets/Scrollbar.cpp
        widgets/Scrollbar.hpp
        widgets/TooltipEntryWidget.cpp
        widgets/TooltipEntryWidget.hpp
        widgets/TooltipWidget.cpp
        widgets/TooltipWidget.hpp
        widgets/Window.cpp
        widgets/Window.hpp

        widgets/dialogs/BadgePickerDialog.cpp
        widgets/dialogs/BadgePickerDialog.hpp
        widgets/dialogs/ChannelFilterEditorDialog.cpp
        widgets/dialogs/ChannelFilterEditorDialog.hpp
        widgets/dialogs/ColorPickerDialog.cpp
        widgets/dialogs/ColorPickerDialog.hpp
        widgets/dialogs/EditHotkeyDialog.cpp
        widgets/dialogs/EditHotkeyDialog.hpp
        widgets/dialogs/EmotePopup.cpp
        widgets/dialogs/EmotePopup.hpp
        widgets/dialogs/LastRunCrashDialog.cpp
        widgets/dialogs/LastRunCrashDialog.hpp
        widgets/dialogs/LoginDialog.cpp
        widgets/dialogs/LoginDialog.hpp
        widgets/dialogs/QualityPopup.cpp
        widgets/dialogs/QualityPopup.hpp
        widgets/dialogs/ReplyThreadPopup.cpp
        widgets/dialogs/ReplyThreadPopup.hpp
        widgets/dialogs/SelectChannelDialog.cpp
        widgets/dialogs/SelectChannelDialog.hpp
        widgets/dialogs/SelectChannelFiltersDialog.cpp
        widgets/dialogs/SelectChannelFiltersDialog.hpp
        widgets/dialogs/SettingsDialog.cpp
        widgets/dialogs/SettingsDialog.hpp
        widgets/dialogs/UpdateDialog.cpp
        widgets/dialogs/UpdateDialog.hpp
        widgets/dialogs/UserInfoPopup.cpp
        widgets/dialogs/UserInfoPopup.hpp
        widgets/dialogs/WelcomeDialog.cpp
        widgets/dialogs/WelcomeDialog.hpp
        widgets/dialogs/switcher/NewPopupItem.cpp
        widgets/dialogs/switcher/NewPopupItem.hpp
        widgets/dialogs/switcher/NewTabItem.cpp
        widgets/dialogs/switcher/NewTabItem.hpp
        widgets/dialogs/switcher/QuickSwitcherModel.cpp
        widgets/dialogs/switcher/QuickSwitcherModel.hpp
        widgets/dialogs/switcher/QuickSwitcherPopup.cpp
        widgets/dialogs/switcher/QuickSwitcherPopup.hpp
        widgets/dialogs/switcher/SwitchSplitItem.cpp
        widgets/dialogs/switcher/SwitchSplitItem.hpp

        widgets/helper/color/AlphaSlider.cpp
        widgets/helper/color/AlphaSlider.hpp
        widgets/helper/color/Checkerboard.cpp
        widgets/helper/color/Checkerboard.hpp
        widgets/helper/color/ColorButton.cpp
        widgets/helper/color/ColorButton.hpp
        widgets/helper/color/ColorInput.cpp
        widgets/helper/color/ColorInput.hpp
        widgets/helper/color/ColorItemDelegate.cpp
        widgets/helper/color/ColorItemDelegate.hpp
        widgets/helper/color/HueSlider.cpp
        widgets/helper/color/HueSlider.hpp
        widgets/helper/color/SBCanvas.cpp
        widgets/helper/color/SBCanvas.hpp

        widgets/helper/Button.cpp
        widgets/helper/Button.hpp
        widgets/helper/ChannelView.cpp
        widgets/helper/ChannelView.hpp
        widgets/helper/ComboBoxItemDelegate.cpp
        widgets/helper/ComboBoxItemDelegate.hpp
        widgets/helper/DebugPopup.cpp
        widgets/helper/DebugPopup.hpp
        widgets/helper/EditableModelView.cpp
        widgets/helper/EditableModelView.hpp
        widgets/helper/EffectLabel.cpp
        widgets/helper/EffectLabel.hpp
        widgets/helper/IconDelegate.cpp
        widgets/helper/IconDelegate.hpp
        widgets/helper/InvisibleSizeGrip.cpp
        widgets/helper/InvisibleSizeGrip.hpp
        widgets/helper/MessageView.cpp
        widgets/helper/MessageView.hpp
        widgets/helper/NotebookButton.cpp
        widgets/helper/NotebookButton.hpp
        widgets/helper/NotebookTab.cpp
        widgets/helper/NotebookTab.hpp
        widgets/helper/OverlayInteraction.cpp
        widgets/helper/OverlayInteraction.hpp
        widgets/helper/RegExpItemDelegate.cpp
        widgets/helper/RegExpItemDelegate.hpp
        widgets/helper/ResizingTextEdit.cpp
        widgets/helper/ResizingTextEdit.hpp
        widgets/helper/ScrollbarHighlight.cpp
        widgets/helper/ScrollbarHighlight.hpp
        widgets/helper/SearchPopup.cpp
        widgets/helper/SearchPopup.hpp
        widgets/helper/SettingsDialogTab.cpp
        widgets/helper/SettingsDialogTab.hpp
        widgets/helper/SignalLabel.cpp
        widgets/helper/SignalLabel.hpp
        widgets/helper/TableStyles.cpp
        widgets/helper/TableStyles.hpp
        widgets/helper/TitlebarButton.cpp
        widgets/helper/TitlebarButton.hpp
        widgets/helper/TitlebarButtons.cpp
        widgets/helper/TitlebarButtons.hpp
        widgets/helper/TrimRegExpValidator.cpp
        widgets/helper/TrimRegExpValidator.hpp

        widgets/layout/FlowLayout.cpp
        widgets/layout/FlowLayout.hpp

        widgets/listview/GenericItemDelegate.cpp
        widgets/listview/GenericItemDelegate.hpp
        widgets/listview/GenericListItem.cpp
        widgets/listview/GenericListItem.hpp
        widgets/listview/GenericListModel.cpp
        widgets/listview/GenericListModel.hpp
        widgets/listview/GenericListView.cpp
        widgets/listview/GenericListView.hpp

        widgets/settingspages/AboutPage.cpp
        widgets/settingspages/AboutPage.hpp
        widgets/settingspages/AccountsPage.cpp
        widgets/settingspages/AccountsPage.hpp
        widgets/settingspages/CommandPage.cpp
        widgets/settingspages/CommandPage.hpp
        widgets/settingspages/ExternalToolsPage.cpp
        widgets/settingspages/ExternalToolsPage.hpp
        widgets/settingspages/FiltersPage.cpp
        widgets/settingspages/FiltersPage.hpp
        widgets/settingspages/GeneralPage.cpp
        widgets/settingspages/GeneralPage.hpp
        widgets/settingspages/GeneralPageView.cpp
        widgets/settingspages/GeneralPageView.hpp
        widgets/settingspages/HighlightingPage.cpp
        widgets/settingspages/HighlightingPage.hpp
        widgets/settingspages/IgnoresPage.cpp
        widgets/settingspages/IgnoresPage.hpp
        widgets/settingspages/KeyboardSettingsPage.cpp
        widgets/settingspages/KeyboardSettingsPage.hpp
        widgets/settingspages/ModerationPage.cpp
        widgets/settingspages/ModerationPage.hpp
        widgets/settingspages/NicknamesPage.cpp
        widgets/settingspages/NicknamesPage.hpp
        widgets/settingspages/NotificationPage.cpp
        widgets/settingspages/NotificationPage.hpp
        widgets/settingspages/PluginsPage.cpp
        widgets/settingspages/PluginsPage.hpp
        widgets/settingspages/SettingsPage.cpp
        widgets/settingspages/SettingsPage.hpp
        widgets/settingspages/SettingWidget.cpp
        widgets/settingspages/SettingWidget.hpp

        widgets/splits/ClosedSplits.cpp
        widgets/splits/ClosedSplits.hpp
        widgets/splits/DraggedSplit.cpp
        widgets/splits/DraggedSplit.hpp
        widgets/splits/InputCompletionItem.cpp
        widgets/splits/InputCompletionItem.hpp
        widgets/splits/InputCompletionPopup.cpp
        widgets/splits/InputCompletionPopup.hpp
        widgets/splits/Split.cpp
        widgets/splits/Split.hpp
        widgets/splits/SplitCommon.hpp
        widgets/splits/SplitContainer.cpp
        widgets/splits/SplitContainer.hpp
        widgets/splits/SplitHeader.cpp
        widgets/splits/SplitHeader.hpp
        widgets/splits/SplitInput.cpp
        widgets/splits/SplitInput.hpp
        widgets/splits/SplitOverlay.cpp
        widgets/splits/SplitOverlay.hpp
        )

if (APPLE)
    set(MACOS_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/chatterino.icns")
    list(APPEND SOURCE_FILES "${MACOS_BUNDLE_ICON_FILE}")
    set_source_files_properties(${MACOS_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    list(APPEND SOURCE_FILES util/MacOsHelpers.mm)
endif ()

# Generate source groups for use in IDEs
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_FILES})

# Add autogenerated files
list(APPEND SOURCE_FILES ${RES_AUTOGEN_FILES})

add_library(${LIBRARY_PROJECT} OBJECT ${SOURCE_FILES})

if(CHATTERINO_PLUGINS)
    target_compile_definitions(${LIBRARY_PROJECT}
        PUBLIC
        CHATTERINO_HAVE_PLUGINS
    )
    message(STATUS "Building Chatterino with lua plugin support enabled.")
endif()

if (CHATTERINO_GENERATE_COVERAGE)
    include(CodeCoverage)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
        append_coverage_compiler_flags_to_target(${LIBRARY_PROJECT})
    endif()
    message(STATUS "project source dir: ${PROJECT_SOURCE_DIR}/src")
    setup_target_for_coverage_gcovr_html(
        NAME coverage
        EXECUTABLE ctest
        EXCLUDE "/usr/include/*"
        EXCLUDE "build*/*"
        # EXCLUDE "lib/*"
        EXCLUDE "lib/certify/*"
        EXCLUDE "lib/expected-lite/*"
        EXCLUDE "lib/googletest/*"
        EXCLUDE "lib/libcommuni/*"
        EXCLUDE "lib/lrucache/*"
        EXCLUDE "lib/lua/*"
        EXCLUDE "lib/magic_enum/*"
        EXCLUDE "lib/miniaudio/*"
        EXCLUDE "lib/qtkeychain/*"
        EXCLUDE "lib/rapidjson/*"
        EXCLUDE "lib/semver/*"
        EXCLUDE "lib/serialize/*"
        EXCLUDE "lib/settings/*"
        EXCLUDE "lib/signals/*"
        EXCLUDE "lib/sol2/*"
        EXCLUDE "lib/websocketpp/*"
        EXCLUDE "lib/WinToast/*"

        EXCLUDE "*/ui_*.h"
        EXCLUDE "*/moc_*.cpp"
        )
endif ()

target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        Qt${MAJOR_QT_VERSION}::Core
        Qt${MAJOR_QT_VERSION}::Widgets
        Qt${MAJOR_QT_VERSION}::Gui
        Qt${MAJOR_QT_VERSION}::Network
        Qt${MAJOR_QT_VERSION}::Svg
        Qt${MAJOR_QT_VERSION}::Concurrent

        LibCommuni::LibCommuni
        Pajlada::Serialize
        Pajlada::Settings
        Pajlada::Signals
        websocketpp::websocketpp
        Threads::Threads
        RapidJSON::RapidJSON
        LRUCache
        MagicEnum
        $<$<BOOL:${WIN32}>:Wtsapi32>
        twitch-eventsub-ws
        BoostCertify
        )
if (CHATTERINO_PLUGINS)
    target_link_libraries(${LIBRARY_PROJECT} PUBLIC lua sol2::sol2)
endif()

if (BUILD_WITH_QTKEYCHAIN)
    target_link_libraries(${LIBRARY_PROJECT}
            PUBLIC
            qt${MAJOR_QT_VERSION}keychain
            )
else()
    target_compile_definitions(${LIBRARY_PROJECT}
        PUBLIC
        NO_QTKEYCHAIN
        )
endif()

# Set the output of TARGET to be 
# - CMAKE_BIN_DIR/lib for libraries
# - CMAKE_BIN_DIR/bin for BINARIES
# an additional argument specifies the subdirectory.
function(set_target_directory_hierarchy TARGET)
    set_target_properties(${TARGET}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${ARGV1}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
    )
endfunction()

if (BUILD_APP)
    if (APPLE)
        add_executable(${EXECUTABLE_PROJECT} ${MACOS_BUNDLE_ICON_FILE} main.cpp)
    else()
        add_executable(${EXECUTABLE_PROJECT} main.cpp)
    endif()

    if(COMMAND add_sanitizers)
        add_sanitizers(${EXECUTABLE_PROJECT})
    else()
        message(WARNING "Sanitizers support is disabled")
    endif()

    if (CHATTERINO_STATIC_QT_BUILD)
        qt_import_plugins(${EXECUTABLE_PROJECT} INCLUDE_BY_TYPE
            platforms Qt::QXcbIntegrationPlugin
            Qt::QMinimalIntegrationPlugin
        )
    endif ()

    target_include_directories(${EXECUTABLE_PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/autogen/)

    target_link_libraries(${EXECUTABLE_PROJECT} PUBLIC ${LIBRARY_PROJECT})

    set_target_directory_hierarchy(${EXECUTABLE_PROJECT})

    if (WIN32)
        if (WINDEPLOYQT_PATH)
            file(TO_CMAKE_PATH "${WINDEPLOYQT_PATH}" WINDEPLOYQT_PATH)
        else()
            if (VCPKG_INSTALLED_DIR AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
                find_program(WINDEPLOYQT_PATH NAMES windeployqt.debug.bat)
            else()
                find_program(WINDEPLOYQT_PATH NAMES windeployqt)
            endif()
        endif()

        if (NOT EXISTS ${WINDEPLOYQT_PATH})
            message(FATAL_ERROR "windeployqt.exe not found")
        endif()

        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(WINDEPLOYQT_MODE --debug)
            get_target_property(QT_CORE_LOC Qt${MAJOR_QT_VERSION}::Core LOCATION_DEBUG)
        else()
            set(WINDEPLOYQT_MODE --release)
            get_target_property(QT_CORE_LOC Qt${MAJOR_QT_VERSION}::Core LOCATION)
        endif()
        get_filename_component(QT_BIN_DIR ${QT_CORE_LOC} DIRECTORY)

        # This assumes the installed CRT is up-to-date (see .CI/deploy-crt.ps1)
        set(WINDEPLOYQT_COMMAND_ARGV "${WINDEPLOYQT_PATH}" "$<TARGET_FILE:${EXECUTABLE_PROJECT}>" ${WINDEPLOYQT_MODE} --no-compiler-runtime --no-translations --no-opengl-sw)
        string(REPLACE ";" " " WINDEPLOYQT_COMMAND "${WINDEPLOYQT_COMMAND_ARGV}")

        install(TARGETS ${EXECUTABLE_PROJECT} 
            RUNTIME_DEPENDENCIES 
                PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
                POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" 
                DIRECTORIES ${QT_BIN_DIR}
            RUNTIME DESTINATION .)

        # Hardcoded list of DLLs to install from Qt - these are marked as optional since they only exist for vcpkg
        install(FILES
            ${QT_BIN_DIR}/jpeg62.dll
            ${QT_BIN_DIR}/libwebpdemux.dll
            ${QT_BIN_DIR}/libwebpmux.dll
            ${QT_BIN_DIR}/libwebp.dll
            ${QT_BIN_DIR}/libsharpyuv.dll
            DESTINATION .
            OPTIONAL)

        install(CODE "message(\"-- Running: ${WINDEPLOYQT_COMMAND} --dir \\\"\${CMAKE_INSTALL_PREFIX}\\\"\")")
        install(CODE "execute_process(COMMAND ${WINDEPLOYQT_COMMAND} --dir \"\${CMAKE_INSTALL_PREFIX}\" COMMAND_ERROR_IS_FATAL ANY)")   
    elseif (APPLE)
        install(TARGETS ${EXECUTABLE_PROJECT}
                RUNTIME DESTINATION bin
                BUNDLE DESTINATION bin
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib/static
                )
    else ()
        install(TARGETS ${EXECUTABLE_PROJECT}
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib/static
                )

        install(FILES ${CMAKE_SOURCE_DIR}/resources/com.chatterino.chatterino.desktop
                DESTINATION share/applications
                )

        install(FILES ${CMAKE_SOURCE_DIR}/resources/icon.png
                RENAME com.chatterino.chatterino.png
                DESTINATION share/icons/hicolor/256x256/apps
                )
    endif ()

    if(CHATTERINO_ENABLE_LTO)
        message(STATUS "Enabling LTO for ${EXECUTABLE_PROJECT}")
        set_property(TARGET ${EXECUTABLE_PROJECT}
            PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif ()

if (USE_PRECOMPILED_HEADERS)
    message(STATUS "Building with precompiled headers")
    target_precompile_headers(${LIBRARY_PROJECT} PRIVATE PrecompiledHeader.hpp)
else ()
    message(STATUS "Building without precompiled headers")
endif ()

# Enable autogeneration of Qts MOC/RCC/UIC
set_target_properties(${LIBRARY_PROJECT}
    PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    )

# The version project has definitions about the build.
# To avoid recompilations because of changing preprocessor definitions,
# this is its own project.
set(VERSION_SOURCE_FILES common/Version.cpp common/Version.hpp)
add_library(${VERSION_PROJECT} STATIC ${VERSION_SOURCE_FILES})
target_compile_definitions(${VERSION_PROJECT} PRIVATE
    $<$<BOOL:${WIN32}>:USEWINSDK>
    $<$<BOOL:${BUILD_WITH_CRASHPAD}>:CHATTERINO_WITH_CRASHPAD>
)

# source group for IDEs
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${VERSION_SOURCE_FILES})
target_include_directories(${VERSION_PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${VERSION_PROJECT} PRIVATE Qt${MAJOR_QT_VERSION}::Core)
target_compile_definitions(${VERSION_PROJECT} PRIVATE
    CHATTERINO_GIT_HASH=\"${GIT_HASH}\"
    CHATTERINO_GIT_RELEASE=\"${GIT_RELEASE}\"
    CHATTERINO_GIT_COMMIT=\"${GIT_COMMIT}\"
    CHATTERINO_GIT_MODIFIED=${GIT_MODIFIED}

    CHATTERINO_CMAKE_GEN_DATE=\"${cmake_gen_date}\"
)

target_link_libraries(${LIBRARY_PROJECT} PRIVATE ${VERSION_PROJECT})

target_compile_definitions(${LIBRARY_PROJECT} PUBLIC
    CHATTERINO
    UNICODE
    AB_CUSTOM_SETTINGS
    IRC_STATIC
    IRC_NAMESPACE=Communi
    $<$<BOOL:${WIN32}>:_WIN32_WINNT=0x0A00> # Windows 10
    $<$<BOOL:${BUILD_TESTS}>:CHATTERINO_WITH_TESTS>
    )

if (USE_SYSTEM_QTKEYCHAIN)
    target_compile_definitions(${LIBRARY_PROJECT} PUBLIC
        CMAKE_BUILD
        )
endif ()
if (WIN32)
    target_compile_definitions(${LIBRARY_PROJECT} PUBLIC
        USEWINSDK
        )
    if (BUILD_APP)
        set_target_properties(${EXECUTABLE_PROJECT} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif ()
endif ()
if (CHATTERINO_DEBUG_NATIVE_MESSAGES)
    target_compile_definitions(${LIBRARY_PROJECT} PRIVATE CHATTERINO_DEBUG_NM)
endif ()

if (MSVC)
    target_compile_options(${LIBRARY_PROJECT} PUBLIC /EHsc /bigobj /utf-8)
endif ()

if (APPLE AND BUILD_APP)
    set_target_properties(${EXECUTABLE_PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)
    set_target_properties(${EXECUTABLE_PROJECT}
        PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Chatterino"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.chatterino"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE chatterino.icns
        )
endif ()

target_include_directories(${LIBRARY_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/autogen/)

# semver dependency https://github.com/Neargye/semver
target_include_directories(${LIBRARY_PROJECT} PUBLIC ${CMAKE_SOURCE_DIR}/lib/semver/include)

# expected-lite dependency https://github.com/martinmoene/expected-lite
target_include_directories(${LIBRARY_PROJECT} PUBLIC ${CMAKE_SOURCE_DIR}/lib/expected-lite/include)

# miniaudio dependency https://github.com/mackron/miniaudio
if (USE_SYSTEM_MINIAUDIO)
    message(STATUS "Building with system miniaudio")
    include(CheckIncludeFileCXX)
    CHECK_INCLUDE_FILE_CXX("miniaudio.h" MINIAUDIO_FOUND)
    if (NOT MINIAUDIO_FOUND)
        message(FATAL_ERROR "miniaudio.h not found on your system")
    endif()
else ()
    target_include_directories(${LIBRARY_PROJECT} PUBLIC ${CMAKE_SOURCE_DIR}/lib/miniaudio)
endif ()

if (UNIX)
    if (CMAKE_DL_LIBS)
        # libdl is a requirement for miniaudio on Linux
        message(STATUS "Linking with CMake DL libs: '${CMAKE_DL_LIBS}'")
        target_link_libraries(${LIBRARY_PROJECT} PUBLIC ${CMAKE_DL_LIBS})
    endif ()

    if (NOT APPLE AND BUILD_WITH_LIBNOTIFY)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(LIBNOTIFY REQUIRED IMPORTED_TARGET libnotify)
        target_link_libraries(${LIBRARY_PROJECT} PRIVATE PkgConfig::LIBNOTIFY)
        target_compile_definitions(${LIBRARY_PROJECT} PUBLIC CHATTERINO_WITH_LIBNOTIFY)
    endif ()
endif ()

if (WIN32)
    target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        WinToast)
endif ()

target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        ${Boost_LIBRARIES}
        )

target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        )

target_include_directories(${LIBRARY_PROJECT} PUBLIC ${RapidJSON_INCLUDE_DIRS})

if (LIBRT)
    target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        ${LIBRT}
        )
endif ()

if (BUILD_WITH_CRASHPAD)
    target_compile_definitions(${LIBRARY_PROJECT} PUBLIC CHATTERINO_WITH_CRASHPAD)
    target_link_libraries(${LIBRARY_PROJECT} PUBLIC crashpad::client)
endif()

# Configure compiler warnings
if (MSVC)
    # Change flags for RelWithDebInfo

    # Default: "/debug /INCREMENTAL"
    # Changes:
    #   - Disable incremental linking to reduce padding
    #   - Enable all optimizations - by default when /DEBUG is specified,
    #     these optimizations will be disabled. We need /DEBUG to generate a PDB.
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/20812 for more details.
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /INCREMENTAL:NO /OPT:REF,ICF,LBR")

    # Use the function inlining level from 'Release' mode (2).
    string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    # Configure warnings

    # Someone adds /W3 before we add /W4.
    # This makes sure, only /W4 is specified.
    string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    # 4100 - "unreferenced formal parameter"
    #        There are a lot of functions and methods where
    #        an argument was given a name but never used.
    #        There's a clang-tidy rule that will catch this
    #        for new/updated functions/methods.
    #
    # 4267 - "possible loss of data in return"
    #        These are implicit conversions from size_t to int/qsizetype.
    #        We don't use size_t in a lot of cases, since
    #        Qt doesn't use it - it uses int (or qsizetype in Qt6).
    #
    # 4458 - "declaration of 'identifier' hides class member"
    #        We have a rule of exclusively using `this->`
    #        to access class members, thus it's fine to reclare a variable
    #        with the same name as a class member.
    target_compile_options(${LIBRARY_PROJECT} PUBLIC
        /W4
        # 5038 - warnings about initialization order
        /w15038
        # 4855 - implicit capture of 'this' via '[=]' is deprecated
        /w14855
        # Disable the following warnings (see reasoning above)
        /wd4100
        /wd4267
        /wd4458
        # Enable updated '__cplusplus' macro - workaround for CMake#18837
        /Zc:__cplusplus
    )
    # Disable min/max macros from Windows.h
    target_compile_definitions(${LIBRARY_PROJECT} PUBLIC NOMINMAX)
else ()
    target_compile_options(${LIBRARY_PROJECT} PUBLIC
        -Wall
        # Disable the following warnings
        -Wno-unused-function
        -Wno-switch
        -Wno-deprecated-declarations
        -Wno-sign-compare

        # Disabling strict-aliasing warnings for now, although we probably want to re-enable this in the future
        -Wno-strict-aliasing

        -Werror=return-type
        -Werror=reorder
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${LIBRARY_PROJECT} PUBLIC
            -Wno-unused-local-typedef
            -Wno-unused-private-field
            -Werror=inconsistent-missing-override
            -Werror=final-dtor-non-final-class
            -Werror=ambiguous-reversed-operator

        )
    else ()
        target_compile_options(${LIBRARY_PROJECT} PUBLIC
            -Wno-class-memaccess
        )
    endif()
endif ()

if(CHATTERINO_ENABLE_LTO)
    message(STATUS "Enabling LTO for ${LIBRARY_PROJECT}")
    set_property(TARGET ${LIBRARY_PROJECT}
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(NOT CHATTERINO_UPDATER)
    message(STATUS "Disabling the updater.")
    target_compile_definitions(${LIBRARY_PROJECT} PUBLIC CHATTERINO_DISABLE_UPDATER)
endif()

if (DOXYGEN_FOUND)
    message(STATUS "Doxygen found, adding doxygen target")
    # output will be in docs/html
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")

    doxygen_add_docs(
        doxygen
        ${CMAKE_CURRENT_LIST_DIR}
    )
endif ()
