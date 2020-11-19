# Exposed build flags:
# from lib/websocketpp.pri
#  - WEBSOCKETPP_PREFIX ($$PWD by default)
#  - WEBSOCKETPP_SYSTEM (1 = true) (unix only)
# from lib/rapidjson.pri
#  - RAPIDJSON_PREFIX ($$PWD by default)
#  - RAPIDJSON_SYSTEM (1 = true) (Linux only, uses pkg-config)
# from lib/boost.pri
#  - BOOST_DIRECTORY (C:\local\boost\ by default) (Windows only)

QT                += widgets core gui network multimedia svg concurrent
CONFIG            += communi
COMMUNI           += core model util

INCLUDEPATH       += src/
TARGET             = chatterino
TEMPLATE           = app
PRECOMPILED_HEADER = src/PrecompiledHeader.hpp
CONFIG            += precompile_header
DEFINES           += CHATTERINO
DEFINES           += AB_CUSTOM_THEME
DEFINES           += AB_CUSTOM_SETTINGS
CONFIG            += AB_NOT_STANDALONE

equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 12) {
    message("Cannot use Qt $${QT_VERSION}")
    error("Use Qt 5.12 or newer")
}

useBreakpad {
    LIBS += -L$$PWD/lib/qBreakpad/handler/build
    include(lib/qBreakpad/qBreakpad.pri)
    DEFINES += C_USE_BREAKPAD
}

# use C++17
CONFIG += c++17

# C++17 backwards compatability
win32-msvc* {
    QMAKE_CXXFLAGS += /std:c++17
} else {
    QMAKE_CXXFLAGS += -std=c++17
}

linux {
    LIBS += -lrt
    QMAKE_LFLAGS += -lrt

    # Enable linking libraries using PKGCONFIG += libraryname
    CONFIG += link_pkgconfig
}

macx {
    INCLUDEPATH += /usr/local/include
    INCLUDEPATH += /usr/local/opt/openssl/include
    LIBS += -L/usr/local/opt/openssl/lib
}

# https://bugreports.qt.io/browse/QTBUG-27018
equals(QMAKE_CXX, "clang++")|equals(QMAKE_CXX, "g++") {
    TARGET = bin/chatterino
}

# Icons
macx:ICON = resources/chatterino.icns
win32:RC_FILE = resources/windows.rc

macx {
    LIBS += -L/usr/local/lib
}

# Set C_DEBUG if it's a debug build
CONFIG(debug, debug|release) {
    DEFINES += C_DEBUG
    DEFINES += QT_DEBUG
} else {
    DEFINES += NDEBUG
}

# Submodules
include(lib/warnings.pri)
include(lib/humanize.pri)
include(lib/libcommuni.pri)
include(lib/websocketpp.pri)
include(lib/wintoast.pri)
include(lib/signals.pri)
include(lib/settings.pri)
include(lib/serialize.pri)
include(lib/winsdk.pri)
include(lib/rapidjson.pri)
include(lib/qtkeychain.pri)

exists( $$OUT_PWD/conanbuildinfo.pri ) {
    message("Using conan packages")
    CONFIG += conan_basic_setup
    include($$OUT_PWD/conanbuildinfo.pri)
    LIBS += -lGdi32
}
else{
    include(lib/boost.pri)
    include(lib/openssl.pri)
}

# Optional feature: QtWebEngine
#exists ($(QTDIR)/include/QtWebEngine/QtWebEngine) {
#    message(Using QWebEngine)
#    QT += webenginewidgets
#    DEFINES += "USEWEBENGINE"
#}

SOURCES += \
    src/Application.cpp \
    src/autogenerated/ResourcesAutogen.cpp \
    src/BaseSettings.cpp \
    src/BaseTheme.cpp \
    src/BrowserExtension.cpp \
    src/common/Args.cpp \
    src/common/Channel.cpp \
    src/common/ChannelChatters.cpp \
    src/common/ChatterinoSetting.cpp \
    src/common/CompletionModel.cpp \
    src/common/Credentials.cpp \
    src/common/DownloadManager.cpp \
    src/common/Env.cpp \
    src/common/LinkParser.cpp \
    src/common/Modes.cpp \
    src/common/NetworkManager.cpp \
    src/common/NetworkPrivate.cpp \
    src/common/NetworkRequest.cpp \
    src/common/NetworkResult.cpp \
    src/common/UsernameSet.cpp \
    src/common/Version.cpp \
    src/common/WindowDescriptors.cpp \
    src/controllers/accounts/Account.cpp \
    src/controllers/accounts/AccountController.cpp \
    src/controllers/accounts/AccountModel.cpp \
    src/controllers/commands/Command.cpp \
    src/controllers/commands/CommandController.cpp \
    src/controllers/commands/CommandModel.cpp \
    src/controllers/filters/FilterModel.cpp \
    src/controllers/filters/parser/FilterParser.cpp \
    src/controllers/filters/parser/Tokenizer.cpp \
    src/controllers/filters/parser/Types.cpp \
    src/controllers/highlights/HighlightBlacklistModel.cpp \
    src/controllers/highlights/HighlightModel.cpp \
    src/controllers/highlights/HighlightPhrase.cpp \
    src/controllers/highlights/UserHighlightModel.cpp \
    src/controllers/ignores/IgnoreModel.cpp \
    src/controllers/moderationactions/ModerationAction.cpp \
    src/controllers/moderationactions/ModerationActionModel.cpp \
    src/controllers/notifications/NotificationController.cpp \
    src/controllers/notifications/NotificationModel.cpp \
    src/controllers/pings/MutedChannelModel.cpp \
    src/controllers/taggedusers/TaggedUser.cpp \
    src/controllers/taggedusers/TaggedUsersModel.cpp \
    src/debug/Benchmark.cpp \
    src/main.cpp \
    src/messages/Emote.cpp \
    src/messages/Image.cpp \
    src/messages/ImageSet.cpp \
    src/messages/layouts/MessageLayout.cpp \
    src/messages/layouts/MessageLayoutContainer.cpp \
    src/messages/layouts/MessageLayoutElement.cpp \
    src/messages/Link.cpp \
    src/messages/Message.cpp \
    src/messages/MessageBuilder.cpp \
    src/messages/MessageColor.cpp \
    src/messages/MessageContainer.cpp \
    src/messages/MessageElement.cpp \
    src/messages/search/AuthorPredicate.cpp \
    src/messages/search/LinkPredicate.cpp \
    src/messages/search/SubstringPredicate.cpp \
    src/messages/SharedMessageBuilder.cpp \
    src/providers/bttv/BttvEmotes.cpp \
    src/providers/bttv/LoadBttvChannelEmote.cpp \
    src/providers/chatterino/ChatterinoBadges.cpp \
    src/providers/colors/ColorProvider.cpp \
    src/providers/emoji/Emojis.cpp \
    src/providers/ffz/FfzBadges.cpp \
    src/providers/ffz/FfzEmotes.cpp \
    src/providers/irc/AbstractIrcServer.cpp \
    src/providers/irc/Irc2.cpp \
    src/providers/irc/IrcAccount.cpp \
    src/providers/irc/IrcChannel2.cpp \
    src/providers/irc/IrcCommands.cpp \
    src/providers/irc/IrcConnection2.cpp \
    src/providers/irc/IrcMessageBuilder.cpp \
    src/providers/irc/IrcServer.cpp \
    src/providers/IvrApi.cpp \
    src/providers/LinkResolver.cpp \
    src/providers/twitch/ChannelPointReward.cpp \
    src/providers/twitch/api/Helix.cpp \
    src/providers/twitch/api/Kraken.cpp \
    src/providers/twitch/IrcMessageHandler.cpp \
    src/providers/twitch/PubsubActions.cpp \
    src/providers/twitch/PubsubClient.cpp \
    src/providers/twitch/PubsubHelpers.cpp \
    src/providers/twitch/TwitchAccount.cpp \
    src/providers/twitch/TwitchAccountManager.cpp \
    src/providers/twitch/TwitchBadge.cpp \
    src/providers/twitch/TwitchBadges.cpp \
    src/providers/twitch/TwitchChannel.cpp \
    src/providers/twitch/TwitchEmotes.cpp \
    src/providers/twitch/TwitchHelpers.cpp \
    src/providers/twitch/TwitchIrcServer.cpp \
    src/providers/twitch/TwitchMessageBuilder.cpp \
    src/providers/twitch/TwitchParseCheerEmotes.cpp \
    src/providers/twitch/TwitchUser.cpp \
    src/RunGui.cpp \
    src/singletons/Badges.cpp \
    src/singletons/Emotes.cpp \
    src/singletons/Fonts.cpp \
    src/singletons/helper/GifTimer.cpp \
    src/singletons/helper/LoggingChannel.cpp \
    src/singletons/Logging.cpp \
    src/singletons/NativeMessaging.cpp \
    src/singletons/Paths.cpp \
    src/singletons/Resources.cpp \
    src/singletons/Settings.cpp \
    src/singletons/Theme.cpp \
    src/singletons/Toasts.cpp \
    src/singletons/TooltipPreviewImage.cpp \
    src/singletons/Updates.cpp \
    src/singletons/WindowManager.cpp \
    src/util/Clipboard.cpp \
    src/util/DebugCount.cpp \
    src/util/FormatTime.cpp \
    src/util/FunctionEventFilter.cpp \
    src/util/FuzzyConvert.cpp \
    src/util/Helpers.cpp \
    src/util/IncognitoBrowser.cpp \
    src/util/InitUpdateButton.cpp \
    src/util/JsonQuery.cpp \
    src/util/LayoutHelper.cpp \
    src/util/NuulsUploader.cpp \
    src/util/RapidjsonHelpers.cpp \
    src/util/StreamerMode.cpp \
    src/util/StreamLink.cpp \
    src/util/Twitch.cpp \
    src/util/WindowsHelper.cpp \
    src/widgets/AccountSwitchPopup.cpp \
    src/widgets/AccountSwitchWidget.cpp \
    src/widgets/AttachedWindow.cpp \
    src/widgets/BasePopup.cpp \
    src/widgets/BaseWidget.cpp \
    src/widgets/BaseWindow.cpp \
    src/widgets/dialogs/ChannelFilterEditorDialog.cpp \
    src/widgets/dialogs/ColorPickerDialog.cpp \
    src/widgets/dialogs/EmotePopup.cpp \
    src/widgets/dialogs/IrcConnectionEditor.cpp \
    src/widgets/dialogs/LastRunCrashDialog.cpp \
    src/widgets/dialogs/LoginDialog.cpp \
    src/widgets/dialogs/NotificationPopup.cpp \
    src/widgets/dialogs/QualityPopup.cpp \
    src/widgets/dialogs/SelectChannelDialog.cpp \
    src/widgets/dialogs/SelectChannelFiltersDialog.cpp \
    src/widgets/dialogs/SettingsDialog.cpp \
    src/widgets/listview/GenericItemDelegate.cpp \
    src/widgets/dialogs/switcher/NewTabItem.cpp \
    src/widgets/dialogs/switcher/QuickSwitcherPopup.cpp \
    src/widgets/dialogs/switcher/SwitchSplitItem.cpp \
    src/widgets/dialogs/TextInputDialog.cpp \
    src/widgets/dialogs/UpdateDialog.cpp \
    src/widgets/dialogs/UserInfoPopup.cpp \
    src/widgets/dialogs/WelcomeDialog.cpp \
    src/widgets/helper/Button.cpp \
    src/widgets/helper/ChannelView.cpp \
    src/widgets/helper/ColorButton.cpp \
    src/widgets/helper/ComboBoxItemDelegate.cpp \
    src/widgets/helper/DebugPopup.cpp \
    src/widgets/helper/EditableModelView.cpp \
    src/widgets/helper/EffectLabel.cpp \
    src/widgets/helper/NotebookButton.cpp \
    src/widgets/helper/NotebookTab.cpp \
    src/widgets/helper/QColorPicker.cpp \
    src/widgets/helper/ResizingTextEdit.cpp \
    src/widgets/helper/ScrollbarHighlight.cpp \
    src/widgets/helper/SearchPopup.cpp \
    src/widgets/helper/SettingsDialogTab.cpp \
    src/widgets/helper/SignalLabel.cpp \
    src/widgets/helper/TitlebarButton.cpp \
    src/widgets/Label.cpp \
    src/widgets/Notebook.cpp \
    src/widgets/Scrollbar.cpp \
    src/widgets/listview/GenericListItem.cpp \
    src/widgets/listview/GenericListModel.cpp \
    src/widgets/listview/GenericListView.cpp \
    src/widgets/settingspages/AboutPage.cpp \
    src/widgets/settingspages/AccountsPage.cpp \
    src/widgets/settingspages/CommandPage.cpp \
    src/widgets/settingspages/ExternalToolsPage.cpp \
    src/widgets/settingspages/FiltersPage.cpp \
    src/widgets/settingspages/GeneralPage.cpp \
    src/widgets/settingspages/GeneralPageView.cpp \
    src/widgets/settingspages/HighlightingPage.cpp \
    src/widgets/settingspages/IgnoresPage.cpp \
    src/widgets/settingspages/KeyboardSettingsPage.cpp \
    src/widgets/settingspages/ModerationPage.cpp \
    src/widgets/settingspages/NotificationPage.cpp \
    src/widgets/settingspages/SettingsPage.cpp \
    src/widgets/splits/ClosedSplits.cpp \
    src/widgets/splits/EmoteInputItem.cpp \
    src/widgets/splits/EmoteInputPopup.cpp \
    src/widgets/splits/Split.cpp \
    src/widgets/splits/SplitContainer.cpp \
    src/widgets/splits/SplitHeader.cpp \
    src/widgets/splits/SplitInput.cpp \
    src/widgets/splits/SplitOverlay.cpp \
    src/widgets/StreamView.cpp \
    src/widgets/TooltipWidget.cpp \
    src/widgets/Window.cpp \

HEADERS += \
    src/Application.hpp \
    src/autogenerated/ResourcesAutogen.hpp \
    src/BaseSettings.hpp \
    src/BaseTheme.hpp \
    src/BrowserExtension.hpp \
    src/common/Aliases.hpp \
    src/common/Args.hpp \
    src/common/Atomic.hpp \
    src/common/Channel.hpp \
    src/common/ChannelChatters.hpp \
    src/common/ChatterinoSetting.hpp \
    src/common/Common.hpp \
    src/common/CompletionModel.hpp \
    src/common/ConcurrentMap.hpp \
    src/common/Credentials.hpp \
    src/common/DownloadManager.hpp \
    src/common/Env.hpp \
    src/common/FlagsEnum.hpp \
    src/common/IrcColors.hpp \
    src/common/LinkParser.hpp \
    src/common/Modes.hpp \
    src/common/NetworkCommon.hpp \
    src/common/NetworkManager.hpp \
    src/common/NetworkPrivate.hpp \
    src/common/NetworkRequest.hpp \
    src/common/NetworkResult.hpp \
    src/common/NullablePtr.hpp \
    src/common/Outcome.hpp \
    src/common/ProviderId.hpp \
    src/common/SignalVector.hpp \
    src/common/SignalVectorModel.hpp \
    src/common/Singleton.hpp \
    src/common/UniqueAccess.hpp \
    src/common/UsernameSet.hpp \
    src/common/Version.hpp \
    src/controllers/accounts/Account.hpp \
    src/controllers/accounts/AccountController.hpp \
    src/controllers/accounts/AccountModel.hpp \
    src/controllers/commands/Command.hpp \
    src/controllers/commands/CommandController.hpp \
    src/controllers/commands/CommandModel.hpp \
    src/controllers/filters/FilterModel.hpp \
    src/controllers/filters/FilterRecord.hpp \
    src/controllers/filters/FilterSet.hpp \
    src/controllers/filters/parser/FilterParser.hpp \
    src/controllers/filters/parser/Tokenizer.hpp \
    src/controllers/filters/parser/Types.hpp \
    src/controllers/highlights/HighlightBlacklistModel.hpp \
    src/controllers/highlights/HighlightBlacklistUser.hpp \
    src/controllers/highlights/HighlightModel.hpp \
    src/controllers/highlights/HighlightPhrase.hpp \
    src/controllers/highlights/UserHighlightModel.hpp \
    src/controllers/ignores/IgnoreController.hpp \
    src/controllers/ignores/IgnoreModel.hpp \
    src/controllers/ignores/IgnorePhrase.hpp \
    src/controllers/moderationactions/ModerationAction.hpp \
    src/controllers/moderationactions/ModerationActionModel.hpp \
    src/controllers/notifications/NotificationController.hpp \
    src/controllers/notifications/NotificationModel.hpp \
    src/controllers/pings/MutedChannelModel.hpp \
    src/controllers/taggedusers/TaggedUser.hpp \
    src/controllers/taggedusers/TaggedUsersModel.hpp \
    src/debug/AssertInGuiThread.hpp \
    src/debug/Benchmark.hpp \
    src/ForwardDecl.hpp \
    src/messages/Emote.hpp \
    src/messages/Image.hpp \
    src/messages/ImageSet.hpp \
    src/messages/layouts/MessageLayout.hpp \
    src/messages/layouts/MessageLayoutContainer.hpp \
    src/messages/layouts/MessageLayoutElement.hpp \
    src/messages/LimitedQueue.hpp \
    src/messages/LimitedQueueSnapshot.hpp \
    src/messages/Link.hpp \
    src/messages/Message.hpp \
    src/messages/MessageBuilder.hpp \
    src/messages/MessageColor.hpp \
    src/messages/MessageContainer.hpp \
    src/messages/MessageElement.hpp \
    src/messages/MessageParseArgs.hpp \
    src/messages/search/AuthorPredicate.hpp \
    src/messages/search/LinkPredicate.hpp \
    src/messages/search/MessagePredicate.hpp \
    src/messages/search/SubstringPredicate.hpp \
    src/messages/Selection.hpp \
    src/messages/SharedMessageBuilder.hpp \
    src/PrecompiledHeader.hpp \
    src/providers/bttv/BttvEmotes.hpp \
    src/providers/bttv/LoadBttvChannelEmote.hpp \
    src/providers/chatterino/ChatterinoBadges.hpp \
    src/providers/colors/ColorProvider.hpp \
    src/providers/emoji/Emojis.hpp \
    src/providers/ffz/FfzBadges.hpp \
    src/providers/ffz/FfzEmotes.hpp \
    src/providers/irc/AbstractIrcServer.hpp \
    src/providers/irc/Irc2.hpp \
    src/providers/irc/IrcAccount.hpp \
    src/providers/irc/IrcChannel2.hpp \
    src/providers/irc/IrcCommands.hpp \
    src/providers/irc/IrcConnection2.hpp \
    src/providers/irc/IrcMessageBuilder.hpp \
    src/providers/irc/IrcServer.hpp \
    src/providers/IvrApi.hpp \
    src/providers/LinkResolver.hpp \
    src/providers/twitch/ChannelPointReward.hpp \
    src/providers/twitch/api/Helix.hpp \
    src/providers/twitch/api/Kraken.hpp \
    src/providers/twitch/EmoteValue.hpp \
    src/providers/twitch/IrcMessageHandler.hpp \
    src/providers/twitch/PubsubActions.hpp \
    src/providers/twitch/PubsubClient.hpp \
    src/providers/twitch/PubsubHelpers.hpp \
    src/providers/twitch/TwitchAccount.hpp \
    src/providers/twitch/TwitchAccountManager.hpp \
    src/providers/twitch/TwitchBadge.hpp \
    src/providers/twitch/TwitchBadges.hpp \
    src/providers/twitch/TwitchChannel.hpp \
    src/providers/twitch/TwitchCommon.hpp \
    src/providers/twitch/TwitchEmotes.hpp \
    src/providers/twitch/TwitchHelpers.hpp \
    src/providers/twitch/TwitchIrcServer.hpp \
    src/providers/twitch/TwitchMessageBuilder.hpp \
    src/providers/twitch/TwitchParseCheerEmotes.hpp \
    src/providers/twitch/TwitchUser.hpp \
    src/RunGui.hpp \
    src/singletons/Badges.hpp \
    src/singletons/Emotes.hpp \
    src/singletons/Fonts.hpp \
    src/singletons/helper/GifTimer.hpp \
    src/singletons/helper/LoggingChannel.hpp \
    src/singletons/Logging.hpp \
    src/singletons/NativeMessaging.hpp \
    src/singletons/Paths.hpp \
    src/singletons/Resources.hpp \
    src/singletons/Settings.hpp \
    src/singletons/Theme.hpp \
    src/singletons/Toasts.hpp \
    src/singletons/TooltipPreviewImage.hpp \
    src/singletons/Updates.hpp \
    src/singletons/WindowManager.hpp \
    src/util/Clamp.hpp \
    src/util/Clipboard.hpp \
    src/util/CombinePath.hpp \
    src/util/ConcurrentMap.hpp \
    src/util/DebugCount.hpp \
    src/util/DistanceBetweenPoints.hpp \
    src/util/FormatTime.hpp \
    src/util/FunctionEventFilter.hpp \
    src/util/FuzzyConvert.hpp \
    src/util/Helpers.hpp \
    src/util/IncognitoBrowser.hpp \
    src/util/InitUpdateButton.hpp \
    src/util/IrcHelpers.hpp \
    src/util/IsBigEndian.hpp \
    src/util/JsonQuery.hpp \
    src/util/LayoutCreator.hpp \
    src/util/LayoutHelper.hpp \
    src/util/NuulsUploader.hpp \
    src/util/Overloaded.hpp \
    src/util/PersistSignalVector.hpp \
    src/util/PostToThread.hpp \
    src/util/QObjectRef.hpp \
    src/util/QStringHash.hpp \
    src/util/rangealgorithm.hpp \
    src/util/RapidjsonHelpers.hpp \
    src/util/RapidJsonSerializeQString.hpp \
    src/util/RemoveScrollAreaBackground.hpp \
    src/util/SampleCheerMessages.hpp \
    src/util/SampleLinks.hpp \
    src/util/SharedPtrElementLess.hpp \
    src/util/Shortcut.hpp \
    src/util/StandardItemHelper.hpp \
    src/util/StreamerMode.hpp \
    src/util/StreamLink.hpp \
    src/util/Twitch.hpp \
    src/util/WindowsHelper.hpp \
    src/widgets/AccountSwitchPopup.hpp \
    src/widgets/AccountSwitchWidget.hpp \
    src/widgets/AttachedWindow.hpp \
    src/widgets/BasePopup.hpp \
    src/widgets/BaseWidget.hpp \
    src/widgets/BaseWindow.hpp \
    src/widgets/dialogs/ChannelFilterEditorDialog.hpp \
    src/widgets/dialogs/ColorPickerDialog.hpp \
    src/widgets/dialogs/EmotePopup.hpp \
    src/widgets/dialogs/IrcConnectionEditor.hpp \
    src/widgets/dialogs/LastRunCrashDialog.hpp \
    src/widgets/dialogs/LoginDialog.hpp \
    src/widgets/dialogs/NotificationPopup.hpp \
    src/widgets/dialogs/QualityPopup.hpp \
    src/widgets/dialogs/SelectChannelDialog.hpp \
    src/widgets/dialogs/SelectChannelFiltersDialog.hpp \
    src/widgets/dialogs/SettingsDialog.hpp \
    src/widgets/dialogs/switcher/AbstractSwitcherItem.hpp \
    src/widgets/listview/GenericItemDelegate.hpp \
    src/widgets/dialogs/switcher/NewTabItem.hpp \
    src/widgets/dialogs/switcher/QuickSwitcherModel.hpp \
    src/widgets/dialogs/switcher/QuickSwitcherPopup.hpp \
    src/widgets/dialogs/switcher/SwitchSplitItem.hpp \
    src/widgets/dialogs/TextInputDialog.hpp \
    src/widgets/dialogs/UpdateDialog.hpp \
    src/widgets/dialogs/UserInfoPopup.hpp \
    src/widgets/dialogs/WelcomeDialog.hpp \
    src/widgets/helper/Button.hpp \
    src/widgets/helper/ChannelView.hpp \
    src/widgets/helper/ColorButton.hpp \
    src/widgets/helper/ComboBoxItemDelegate.hpp \
    src/widgets/helper/CommonTexts.hpp \
    src/widgets/helper/DebugPopup.hpp \
    src/widgets/helper/EditableModelView.hpp \
    src/widgets/helper/EffectLabel.hpp \
    src/widgets/helper/Line.hpp \
    src/widgets/helper/NotebookButton.hpp \
    src/widgets/helper/NotebookTab.hpp \
    src/widgets/helper/QColorPicker.hpp \
    src/widgets/helper/ResizingTextEdit.hpp \
    src/widgets/helper/ScrollbarHighlight.hpp \
    src/widgets/helper/SearchPopup.hpp \
    src/widgets/helper/SettingsDialogTab.hpp \
    src/widgets/helper/SignalLabel.hpp \
    src/widgets/helper/TitlebarButton.hpp \
    src/widgets/Label.hpp \
    src/widgets/Notebook.hpp \
    src/widgets/Scrollbar.hpp \
    src/widgets/listview/GenericListItem.hpp \
    src/widgets/listview/GenericListModel.hpp \
    src/widgets/listview/GenericListView.hpp \
    src/widgets/settingspages/AboutPage.hpp \
    src/widgets/settingspages/AccountsPage.hpp \
    src/widgets/settingspages/CommandPage.hpp \
    src/widgets/settingspages/ExternalToolsPage.hpp \
    src/widgets/settingspages/FiltersPage.hpp \
    src/widgets/settingspages/GeneralPage.hpp \
    src/widgets/settingspages/GeneralPageView.hpp \
    src/widgets/settingspages/HighlightingPage.hpp \
    src/widgets/settingspages/IgnoresPage.hpp \
    src/widgets/settingspages/KeyboardSettingsPage.hpp \
    src/widgets/settingspages/ModerationPage.hpp \
    src/widgets/settingspages/NotificationPage.hpp \
    src/widgets/settingspages/SettingsPage.hpp \
    src/widgets/splits/ClosedSplits.hpp \
    src/widgets/splits/EmoteInputItem.hpp \
    src/widgets/splits/EmoteInputPopup.hpp \
    src/widgets/splits/Split.hpp \
    src/widgets/splits/SplitContainer.hpp \
    src/widgets/splits/SplitHeader.hpp \
    src/widgets/splits/SplitInput.hpp \
    src/widgets/splits/SplitOverlay.hpp \
    src/widgets/StreamView.hpp \
    src/widgets/TooltipWidget.hpp \
    src/widgets/Window.hpp \

RESOURCES += \
    resources/resources.qrc \
    resources/resources_autogenerated.qrc

DISTFILES +=

FORMS += \
    src/widgets/dialogs/IrcConnectionEditor.ui

# do not use windows min/max macros
#win32 {
#    DEFINES += NOMINMAX
#}

linux:isEmpty(PREFIX) {
    message("Using default installation prefix (/usr/local). Change PREFIX in qmake command")
    PREFIX = /usr/local
}

linux {
    desktop.files = resources/com.chatterino.chatterino.desktop
    desktop.path = $$PREFIX/share/applications

    build_icons.path = .
    build_icons.commands = @echo $$PWD  && mkdir -p $$PWD/resources/linuxinstall/icons/hicolor/256x256 && cp $$PWD/resources/icon.png $$PWD/resources/linuxinstall/icons/hicolor/256x256/com.chatterino.chatterino.png

    icon.files = $$PWD/resources/linuxinstall/icons/hicolor/256x256/com.chatterino.chatterino.png
    icon.path = $$PREFIX/share/icons/hicolor/256x256/apps

    target.path = $$PREFIX/bin

    INSTALLS += desktop build_icons icon target
}

git_commit=$$(GIT_COMMIT)
git_release=$$(GIT_RELEASE)
# Git data
isEmpty(git_commit) {
git_commit=$$system(git rev-parse HEAD)
}
isEmpty(git_release) {
git_release=$$system(git describe)
}
git_hash = $$str_member($$git_commit, 0, 8)

# Passing strings as defines requires you to use this weird triple-escape then quotation mark syntax.
# https://stackoverflow.com/questions/3348711/add-a-define-to-qmake-with-a-value/18343449#18343449
DEFINES += CHATTERINO_GIT_COMMIT=\\\"$$git_commit\\\"
DEFINES += CHATTERINO_GIT_RELEASE=\\\"$$git_release\\\"
DEFINES += CHATTERINO_GIT_HASH=\\\"$$git_hash\\\"

CONFIG(debug, debug|release) {
    message("Building Chatterino2 DEBUG")
} else {
    message("Building Chatterino2 RELEASE")
}

message("Injected git values: $$git_commit ($$git_release) $$git_hash")
