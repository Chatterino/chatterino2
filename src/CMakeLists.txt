set(LIBRARY_PROJECT "${PROJECT_NAME}-lib")
set(EXECUTABLE_PROJECT "${PROJECT_NAME}")

set(SOURCE_FILES
        Application.cpp
        Application.hpp
        BaseSettings.cpp
        BaseSettings.hpp
        BaseTheme.cpp
        BaseTheme.hpp
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
        common/CompletionModel.cpp
        common/CompletionModel.hpp
        common/Credentials.cpp
        common/Credentials.hpp
        common/DownloadManager.cpp
        common/DownloadManager.hpp
        common/Env.cpp
        common/Env.hpp
        common/LinkParser.cpp
        common/LinkParser.hpp
        common/Modes.cpp
        common/Modes.hpp
        common/NetworkCommon.cpp
        common/NetworkCommon.hpp
        common/NetworkManager.cpp
        common/NetworkManager.hpp
        common/NetworkPrivate.cpp
        common/NetworkPrivate.hpp
        common/NetworkRequest.cpp
        common/NetworkRequest.hpp
        common/NetworkResult.cpp
        common/NetworkResult.hpp
        common/QLogging.cpp
        common/QLogging.hpp
        common/Version.cpp
        common/Version.hpp
        common/WindowDescriptors.cpp
        common/WindowDescriptors.hpp

        controllers/accounts/Account.cpp
        controllers/accounts/Account.hpp
        controllers/accounts/AccountController.cpp
        controllers/accounts/AccountController.hpp
        controllers/accounts/AccountModel.cpp
        controllers/accounts/AccountModel.hpp

        controllers/commands/Command.cpp
        controllers/commands/Command.hpp
        controllers/commands/CommandController.cpp
        controllers/commands/CommandController.hpp
        controllers/commands/CommandModel.cpp
        controllers/commands/CommandModel.hpp

        controllers/filters/FilterModel.cpp
        controllers/filters/FilterModel.hpp
        controllers/filters/parser/FilterParser.cpp
        controllers/filters/parser/FilterParser.hpp
        controllers/filters/parser/Tokenizer.cpp
        controllers/filters/parser/Tokenizer.hpp
        controllers/filters/parser/Types.cpp
        controllers/filters/parser/Types.hpp

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

        controllers/moderationactions/ModerationAction.cpp
        controllers/moderationactions/ModerationAction.hpp
        controllers/moderationactions/ModerationActionModel.cpp
        controllers/moderationactions/ModerationActionModel.hpp

        controllers/nicknames/NicknamesModel.cpp
        controllers/nicknames/NicknamesModel.hpp
        controllers/nicknames/Nickname.hpp

        controllers/notifications/NotificationController.cpp
        controllers/notifications/NotificationController.hpp
        controllers/notifications/NotificationModel.cpp
        controllers/notifications/NotificationModel.hpp

        controllers/pings/MutedChannelModel.cpp
        controllers/pings/MutedChannelModel.hpp

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
        messages/MessageThread.cpp
        messages/MessageThread.hpp

        messages/SharedMessageBuilder.cpp
        messages/SharedMessageBuilder.hpp

        messages/layouts/MessageLayout.cpp
        messages/layouts/MessageLayout.hpp
        messages/layouts/MessageLayoutContainer.cpp
        messages/layouts/MessageLayoutContainer.hpp
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
        providers/LinkResolver.cpp
        providers/LinkResolver.hpp
        providers/RecentMessagesApi.cpp
        providers/RecentMessagesApi.hpp

        providers/bttv/BttvEmotes.cpp
        providers/bttv/BttvEmotes.hpp
        providers/bttv/LoadBttvChannelEmote.cpp
        providers/bttv/LoadBttvChannelEmote.hpp

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

        providers/irc/AbstractIrcServer.cpp
        providers/irc/AbstractIrcServer.hpp
        providers/irc/Irc2.cpp
        providers/irc/Irc2.hpp
        providers/irc/IrcAccount.cpp
        providers/irc/IrcAccount.hpp
        providers/irc/IrcChannel2.cpp
        providers/irc/IrcChannel2.hpp
        providers/irc/IrcCommands.cpp
        providers/irc/IrcCommands.hpp
        providers/irc/IrcConnection2.cpp
        providers/irc/IrcConnection2.hpp
        providers/irc/IrcMessageBuilder.cpp
        providers/irc/IrcMessageBuilder.hpp
        providers/irc/IrcServer.cpp
        providers/irc/IrcServer.hpp

        providers/seventv/SeventvBadges.cpp
        providers/seventv/SeventvBadges.hpp
        providers/seventv/SeventvEmotes.cpp
        providers/seventv/SeventvEmotes.hpp

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
        providers/twitch/TwitchEmotes.cpp
        providers/twitch/TwitchEmotes.hpp
        providers/twitch/TwitchHelpers.cpp
        providers/twitch/TwitchHelpers.hpp
        providers/twitch/TwitchIrcServer.cpp
        providers/twitch/TwitchIrcServer.hpp
        providers/twitch/TwitchMessageBuilder.cpp
        providers/twitch/TwitchMessageBuilder.hpp
        providers/twitch/TwitchUser.cpp
        providers/twitch/TwitchUser.hpp

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
        providers/twitch/pubsubmessages/Message.hpp
        providers/twitch/pubsubmessages/Unlisten.cpp
        providers/twitch/pubsubmessages/Unlisten.hpp
        providers/twitch/pubsubmessages/Whisper.cpp
        providers/twitch/pubsubmessages/Whisper.hpp

        providers/twitch/api/Helix.cpp
        providers/twitch/api/Helix.hpp

        singletons/Badges.cpp
        singletons/Badges.hpp
        singletons/Emotes.cpp
        singletons/Emotes.hpp
        singletons/Fonts.cpp
        singletons/Fonts.hpp
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
        singletons/Theme.cpp
        singletons/Theme.hpp
        singletons/Toasts.cpp
        singletons/Toasts.hpp
        singletons/TooltipPreviewImage.cpp
        singletons/TooltipPreviewImage.hpp
        singletons/Updates.cpp
        singletons/Updates.hpp
        singletons/WindowManager.cpp
        singletons/WindowManager.hpp

        singletons/helper/GifTimer.cpp
        singletons/helper/GifTimer.hpp
        singletons/helper/LoggingChannel.cpp
        singletons/helper/LoggingChannel.hpp

        util/AttachToConsole.cpp
        util/AttachToConsole.hpp
        util/Clipboard.cpp
        util/Clipboard.hpp
        util/DebugCount.cpp
        util/DebugCount.hpp
        util/DisplayBadge.cpp
        util/DisplayBadge.hpp
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
        util/LayoutHelper.cpp
        util/LayoutHelper.hpp
        util/NuulsUploader.cpp
        util/NuulsUploader.hpp
        util/RapidjsonHelpers.cpp
        util/RapidjsonHelpers.hpp
        util/RatelimitBucket.cpp
        util/RatelimitBucket.hpp
        util/SampleData.cpp
        util/SampleData.hpp
        util/SplitCommand.cpp
        util/SplitCommand.hpp
        util/StreamLink.cpp
        util/StreamLink.hpp
        util/StreamerMode.cpp
        util/StreamerMode.hpp
        util/Twitch.cpp
        util/Twitch.hpp
        util/TypeName.hpp
        util/WindowsHelper.cpp
        util/WindowsHelper.hpp

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
        widgets/Scrollbar.cpp
        widgets/Scrollbar.hpp
        widgets/StreamView.cpp
        widgets/StreamView.hpp
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
        widgets/dialogs/IrcConnectionEditor.cpp
        widgets/dialogs/IrcConnectionEditor.hpp
        widgets/dialogs/IrcConnectionEditor.ui
        widgets/dialogs/LastRunCrashDialog.cpp
        widgets/dialogs/LastRunCrashDialog.hpp
        widgets/dialogs/LoginDialog.cpp
        widgets/dialogs/LoginDialog.hpp
        widgets/dialogs/NotificationPopup.cpp
        widgets/dialogs/NotificationPopup.hpp
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

        widgets/helper/Button.cpp
        widgets/helper/Button.hpp
        widgets/helper/ChannelView.cpp
        widgets/helper/ChannelView.hpp
        widgets/helper/ColorButton.cpp
        widgets/helper/ColorButton.hpp
        widgets/helper/ComboBoxItemDelegate.cpp
        widgets/helper/ComboBoxItemDelegate.hpp
        widgets/helper/DebugPopup.cpp
        widgets/helper/DebugPopup.hpp
        widgets/helper/EditableModelView.cpp
        widgets/helper/EditableModelView.hpp
        widgets/helper/EffectLabel.cpp
        widgets/helper/EffectLabel.hpp
        widgets/helper/NotebookButton.cpp
        widgets/helper/NotebookButton.hpp
        widgets/helper/NotebookTab.cpp
        widgets/helper/NotebookTab.hpp
        widgets/helper/QColorPicker.cpp
        widgets/helper/QColorPicker.hpp
        widgets/helper/RegExpItemDelegate.cpp
        widgets/helper/RegExpItemDelegate.hpp
        widgets/helper/TrimRegExpValidator.cpp
        widgets/helper/TrimRegExpValidator.hpp
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
        widgets/helper/TitlebarButton.cpp
        widgets/helper/TitlebarButton.hpp

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
        widgets/settingspages/SettingsPage.cpp
        widgets/settingspages/SettingsPage.hpp

        widgets/splits/ClosedSplits.cpp
        widgets/splits/ClosedSplits.hpp
        widgets/splits/InputCompletionItem.cpp
        widgets/splits/InputCompletionItem.hpp
        widgets/splits/InputCompletionPopup.cpp
        widgets/splits/InputCompletionPopup.hpp
        widgets/splits/Split.cpp
        widgets/splits/Split.hpp
        widgets/splits/SplitContainer.cpp
        widgets/splits/SplitContainer.hpp
        widgets/splits/SplitHeader.cpp
        widgets/splits/SplitHeader.hpp
        widgets/splits/SplitInput.cpp
        widgets/splits/SplitInput.hpp
        widgets/splits/SplitOverlay.cpp
        widgets/splits/SplitOverlay.hpp

        autogenerated/ResourcesAutogen.cpp
        autogenerated/ResourcesAutogen.hpp

        ${CMAKE_SOURCE_DIR}/resources/resources.qrc
        ${CMAKE_SOURCE_DIR}/resources/resources_autogenerated.qrc
        )

if (WIN32)
    # clang-cl doesn't support resource files
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND SOURCE_FILES "${CMAKE_SOURCE_DIR}/resources/windows.rc")
    endif ()

elseif (APPLE)
    set(MACOS_BUNDLE_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/chatterino.icns")
    list(APPEND SOURCE_FILES "${MACOS_BUNDLE_ICON_FILE}")
    set_source_files_properties(${MACOS_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif ()

# Generate source groups for use in IDEs
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_FILES})

add_library(${LIBRARY_PROJECT} OBJECT ${SOURCE_FILES})

target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        Qt${MAJOR_QT_VERSION}::Core
        Qt${MAJOR_QT_VERSION}::Widgets
        Qt${MAJOR_QT_VERSION}::Gui
        Qt${MAJOR_QT_VERSION}::Network
        Qt${MAJOR_QT_VERSION}::Multimedia
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
        )
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

if (BUILD_APP)
    if (APPLE)
        add_executable(${EXECUTABLE_PROJECT} ${MACOS_BUNDLE_ICON_FILE} main.cpp)
    else()
        add_executable(${EXECUTABLE_PROJECT} main.cpp)
    endif()
    add_sanitizers(${EXECUTABLE_PROJECT})

    target_include_directories(${EXECUTABLE_PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(${EXECUTABLE_PROJECT} PUBLIC ${LIBRARY_PROJECT})

    set_target_properties(${EXECUTABLE_PROJECT}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
        )

    if (MSVC)
        get_target_property(Qt_Core_Location Qt${MAJOR_QT_VERSION}::Core LOCATION)
        get_filename_component(QT_BIN_DIR ${Qt_Core_Location} DIRECTORY)
        set(WINDEPLOYQT_COMMAND "${QT_BIN_DIR}/windeployqt.exe" $<TARGET_FILE:${EXECUTABLE_PROJECT}> --release --no-compiler-runtime --no-translations --no-opengl-sw)

        install(TARGETS ${EXECUTABLE_PROJECT}
                RUNTIME DESTINATION .
                )

        install(CODE "execute_process(COMMAND ${WINDEPLOYQT_COMMAND} --dir \${CMAKE_INSTALL_PREFIX})")
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

# Used to provide a date of build in the About page (for nightly builds). Getting the actual time of
# compilation in CMake is a more involved, as documented in https://stackoverflow.com/q/24292898.
# For CI runs, however, the date of build file generation should be consistent with the date of
# compilation so this approximation is "good enough" for our purpose.
if (DEFINED ENV{CHATTERINO_SKIP_DATE_GEN})
    set(cmake_gen_date "1970-01-01")
else ()
    string(TIMESTAMP cmake_gen_date "%Y-%m-%d")
endif ()

target_compile_definitions(${LIBRARY_PROJECT} PUBLIC
    CHATTERINO
    UNICODE
    AB_CUSTOM_THEME
    AB_CUSTOM_SETTINGS
    IRC_STATIC
    IRC_NAMESPACE=Communi

    CHATTERINO_GIT_HASH=\"${GIT_HASH}\"
    CHATTERINO_GIT_RELEASE=\"${GIT_RELEASE}\"
    CHATTERINO_GIT_COMMIT=\"${GIT_COMMIT}\"

    CHATTERINO_CMAKE_GEN_DATE=\"${cmake_gen_date}\"
    )
if (GIT_MODIFIED)
    target_compile_definitions(${LIBRARY_PROJECT} PUBLIC
        CHATTERINO_GIT_MODIFIED
        )
endif ()
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

if (MSVC)
    target_compile_options(${LIBRARY_PROJECT} PUBLIC /EHsc /bigobj)
endif ()

if (APPLE AND BUILD_APP)
    set_target_properties(${EXECUTABLE_PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)
    set_target_properties(${EXECUTABLE_PROJECT}
        PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Chatterino"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.chatterino"
        MACOSX_BUNDLE_INFO_STRING "Chat client for Twitch"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE chatterino.icns
        )
endif ()

target_include_directories(${LIBRARY_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WinToast_FOUND)
    target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        WinToast)
endif ()

if (USE_CONAN AND TARGET CONAN_PKG::boost)
    target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        CONAN_PKG::boost
        )
else ()
    target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        ${Boost_LIBRARIES}
        )
endif ()

if (USE_CONAN AND TARGET CONAN_PKG::openssl)
    target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        CONAN_PKG::openssl
        )
else ()
    target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        )
endif ()

target_include_directories(${LIBRARY_PROJECT} PUBLIC ${RapidJSON_INCLUDE_DIRS})

if (LIBRT)
    target_link_libraries(${LIBRARY_PROJECT}
        PUBLIC
        ${LIBRT}
        )
endif ()

# Configure compiler warnings
if (MSVC)
    # 4714 - function marked as __forceinline not inlined
    # 4996 - occurs when the compiler encounters a function or variable that is marked as deprecated.
    #        These functions may have a different preferred name, may be insecure or have
    #        a more secure variant, or may be obsolete.
    # 4505 - unreferenced local version has been removed
    # 4127 - conditional expression is constant
    # 4503 - decorated name length exceeded, name was truncated
    # 4100 - unreferences formal parameter
    # 4305 - possible truncation of data
    # 4267 - possible loss of data in return
    target_compile_options(${LIBRARY_PROJECT} PUBLIC
        /W4
        # Disable the following warnings
        /wd4714
        /wd4996
        /wd4505
        /wd4127
        /wd4503
        /wd4100
        /wd4305
        /wd4267
    )
else ()
    target_compile_options(${LIBRARY_PROJECT} PUBLIC
        -Wall
        # Disable the following warnings
        -Wno-unused-function
        -Wno-switch
        -Wno-deprecated-declarations
        -Wno-sign-compare
        -Wno-unused-variable

        # Disabling strict-aliasing warnings for now, although we probably want to re-enable this in the future
        -Wno-strict-aliasing

        -Werror=return-type
        -Werror=reorder
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${LIBRARY_PROJECT} PUBLIC
            -Wno-unused-local-typedef
            -Wno-unused-private-field
        )
    else ()
        target_compile_options(${LIBRARY_PROJECT} PUBLIC
            -Wno-class-memaccess
        )
    endif()
endif ()
