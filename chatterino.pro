#-------------------------------------------------
#
# Project created by QtCreator 2016-12-28T18:23:35
#
#-------------------------------------------------

message(----)

QT                += widgets core gui network multimedia svg
CONFIG            += communi
COMMUNI           += core model util
CONFIG            += c++14
INCLUDEPATH       += src/
TARGET             = chatterino
TEMPLATE           = app
DEFINES           += QT_DEPRECATED_WARNINGS
PRECOMPILED_HEADER = src/PrecompiledHeader.hpp
CONFIG            += precompile_header

useBreakpad {
    LIBS += -L$$PWD/lib/qBreakpad/handler/build
    include(lib/qBreakpad/qBreakpad.pri)
    DEFINES += C_USE_BREAKPAD
}

# https://bugreports.qt.io/browse/QTBUG-27018
equals(QMAKE_CXX, "clang++")|equals(QMAKE_CXX, "g++") {
    TARGET = bin/chatterino
}

# Icons
macx:ICON = resources/images/chatterino2.icns
win32:RC_FILE = resources/windows.rc


macx {
    LIBS += -L/usr/local/lib
}

# Submodules
include(dependencies/rapidjson.pri)
include(dependencies/settings.pri)
include(dependencies/signals.pri)
include(dependencies/humanize.pri)
include(dependencies/fmt.pri)
DEFINES += IRC_NAMESPACE=Communi
include(dependencies/libcommuni.pri)
include(dependencies/websocketpp.pri)
include(dependencies/openssl.pri)
include(dependencies/boost.pri)

# Optional feature: QtWebEngine
#exists ($(QTDIR)/include/QtWebEngine/QtWebEngine) {
#    message(Using QWebEngine)
#    QT += webenginewidgets
#    DEFINES += "USEWEBENGINE"
#}

linux {
    LIBS += -lrt
}

win32 {
    LIBS += -luser32
}

# OSX include directory
macx {
    INCLUDEPATH += /usr/local/include
}

# Optional dependency on Windows SDK 7
!contains(QMAKE_TARGET.arch, x86_64) {
    win32:exists(C:\Program Files\Microsoft SDKs\Windows\v7.1\Include\Windows.h) {
        LIBS += -L"C:\Program Files\Microsoft SDKs\Windows\v7.1\Lib" \
            -ldwmapi

        DEFINES += "USEWINSDK"
        message(Using Windows SDK 7)
    }
}

# Optional dependency on Windows SDK 10
contains(QMAKE_TARGET.arch, x86_64) {
    WIN_SDK_VERSION = $$(WindowsSDKVersion)
    !isEmpty(WIN_SDK_VERSION) {
        !equals(WIN_SDK_VERSION, "\\") {
            DEFINES += "USEWINSDK"
            message(Using Windows SDK 10)
        }
    }
}

werr {
    QMAKE_CXXFLAGS += -Werror

    message("Enabling error on warning")
}

SOURCES += \
    src/Application.cpp \
    src/common/Channel.cpp \
    src/common/CompletionModel.cpp \
    src/common/Emotemap.cpp \
    src/common/NetworkManager.cpp \
    src/common/NetworkRequest.cpp \
    src/controllers/accounts/Account.cpp \
    src/controllers/accounts/AccountController.cpp \
    src/controllers/accounts/AccountModel.cpp \
    src/controllers/commands/Command.cpp \
    src/controllers/commands/CommandController.cpp \
    src/controllers/commands/CommandModel.cpp \
    src/controllers/highlights/HighlightController.cpp \
    src/controllers/highlights/HighlightModel.cpp \
    src/controllers/highlights/HighlightBlacklistModel.cpp \
    src/controllers/highlights/UserHighlightModel.cpp \
    src/controllers/ignores/IgnoreController.cpp \
    src/controllers/ignores/IgnoreModel.cpp \
    src/controllers/taggedusers/TaggedUser.cpp \
    src/controllers/taggedusers/TaggedUsersController.cpp \
    src/controllers/taggedusers/TaggedUsersModel.cpp \
    src/main.cpp \
    src/messages/Image.cpp \
    src/messages/layouts/MessageLayout.cpp \
    src/messages/layouts/MessageLayoutContainer.cpp \
    src/messages/layouts/MessageLayoutElement.cpp \
    src/messages/Link.cpp \
    src/messages/Message.cpp \
    src/messages/MessageBuilder.cpp \
    src/messages/MessageColor.cpp \
    src/messages/MessageElement.cpp \
    src/providers/bttv/BttvEmotes.cpp \
    src/providers/emoji/Emojis.cpp \
    src/providers/ffz/FfzEmotes.cpp \
    src/providers/irc/AbstractIrcServer.cpp \
    src/providers/irc/IrcAccount.cpp \
    src/providers/irc/IrcChannel2.cpp \
    src/providers/irc/IrcConnection2.cpp \
    src/providers/irc/IrcServer.cpp \
    src/providers/twitch/IrcMessageHandler.cpp \
    src/providers/twitch/Pubsub.cpp \
    src/providers/twitch/PubsubActions.cpp \
    src/providers/twitch/PubsubHelpers.cpp \
    src/providers/twitch/TwitchAccount.cpp \
    src/providers/twitch/TwitchAccountManager.cpp \
    src/providers/twitch/TwitchChannel.cpp \
    src/providers/twitch/TwitchEmotes.cpp \
    src/providers/twitch/TwitchHelpers.cpp \
    src/providers/twitch/TwitchMessageBuilder.cpp \
    src/providers/twitch/TwitchServer.cpp \
    src/providers/twitch/TwitchUser.cpp \
    src/common/ChatterinoSetting.cpp \
    src/singletons/helper/GifTimer.cpp \
    src/singletons/helper/LoggingChannel.cpp \
    src/controllers/moderationactions/ModerationAction.cpp \
    src/singletons/WindowManager.cpp \
    src/util/DebugCount.cpp \
    src/util/RapidjsonHelpers.cpp \
    src/util/StreamLink.cpp \
    src/util/WindowsHelper.cpp \
    src/widgets/AccountSwitchPopupWidget.cpp \
    src/widgets/AccountSwitchWidget.cpp \
    src/widgets/AttachedWindow.cpp \
    src/widgets/BaseWidget.cpp \
    src/widgets/BaseWindow.cpp \
    src/widgets/dialogs/EmotePopup.cpp \
    src/widgets/dialogs/LastRunCrashDialog.cpp \
    src/widgets/dialogs/LoginDialog.cpp \
    src/widgets/dialogs/LogsPopup.cpp \
    src/widgets/dialogs/NotificationPopup.cpp \
    src/widgets/dialogs/QualityPopup.cpp \
    src/widgets/dialogs/SelectChannelDialog.cpp \
    src/widgets/dialogs/SettingsDialog.cpp \
    src/widgets/dialogs/TextInputDialog.cpp \
    src/widgets/dialogs/UserInfoPopup.cpp \
    src/widgets/dialogs/WelcomeDialog.cpp \
    src/widgets/helper/ChannelView.cpp \
    src/widgets/helper/ComboBoxItemDelegate.cpp \
    src/widgets/helper/DebugPopup.cpp \
    src/widgets/helper/DropOverlay.cpp \
    src/widgets/helper/DropPreview.cpp \
    src/widgets/helper/EditableModelView.cpp \
    src/widgets/helper/NotebookButton.cpp \
    src/widgets/helper/NotebookTab.cpp \
    src/widgets/helper/ResizingTextEdit.cpp \
    src/widgets/helper/RippleEffectButton.cpp \
    src/widgets/helper/RippleEffectLabel.cpp \
    src/widgets/helper/ScrollbarHighlight.cpp \
    src/widgets/helper/SearchPopup.cpp \
    src/widgets/helper/SettingsDialogTab.cpp \
    src/widgets/helper/SignalLabel.cpp \
    src/widgets/helper/TitlebarButton.cpp \
    src/widgets/Label.cpp \
    src/widgets/Notebook.cpp \
    src/widgets/Scrollbar.cpp \
    src/widgets/settingspages/AboutPage.cpp \
    src/widgets/settingspages/AccountsPage.cpp \
    src/widgets/settingspages/BrowserExtensionPage.cpp \
    src/widgets/settingspages/CommandPage.cpp \
    src/widgets/settingspages/EmotesPage.cpp \
    src/widgets/settingspages/ExternalToolsPage.cpp \
    src/widgets/settingspages/HighlightingPage.cpp \
    src/widgets/settingspages/KeyboardSettingsPage.cpp \
    src/widgets/settingspages/LogsPage.cpp \
    src/widgets/settingspages/ModerationPage.cpp \
    src/widgets/settingspages/SettingsPage.cpp \
    src/widgets/settingspages/SpecialChannelsPage.cpp \
    src/widgets/splits/Split.cpp \
    src/widgets/splits/SplitContainer.cpp \
    src/widgets/splits/SplitHeader.cpp \
    src/widgets/splits/SplitInput.cpp \
    src/widgets/splits/SplitOverlay.cpp \
    src/widgets/StreamView.cpp \
    src/widgets/TooltipWidget.cpp \
    src/widgets/Window.cpp \
    src/common/LinkParser.cpp \
    src/controllers/moderationactions/ModerationActions.cpp \
    src/singletons/NativeMessaging.cpp \
    src/singletons/Emotes.cpp \
    src/singletons/Fonts.cpp \
    src/singletons/Logging.cpp \
    src/singletons/Paths.cpp \
    src/singletons/Resources.cpp \
    src/singletons/Settings.cpp \
    src/singletons/Updates.cpp \
    src/singletons/Theme.cpp \
    src/controllers/moderationactions/ModerationActionModel.cpp \
    src/widgets/settingspages/LookPage.cpp \
    src/widgets/settingspages/FeelPage.cpp \
    src/util/InitUpdateButton.cpp \
    src/widgets/dialogs/UpdateDialog.cpp \
    src/widgets/settingspages/IgnoresPage.cpp

HEADERS  += \
    src/Application.hpp \
    src/common/Channel.hpp \
    src/common/Common.hpp \
    src/common/CompletionModel.hpp \
    src/common/Emotemap.hpp \
    src/common/FlagsEnum.hpp \
    src/common/LockedObject.hpp \
    src/common/MutexValue.hpp \
    src/common/NetworkManager.hpp \
    src/common/NetworkRequest.hpp \
    src/common/NetworkRequester.hpp \
    src/common/NetworkWorker.hpp \
    src/common/NullablePtr.hpp \
    src/common/Property.hpp \
    src/common/ProviderId.hpp \
    src/common/SerializeCustom.hpp \
    src/common/SignalVectorModel.hpp \
    src/common/UrlFetch.hpp \
    src/common/Version.hpp \
    src/controllers/accounts/Account.hpp \
    src/controllers/accounts/AccountController.hpp \
    src/controllers/accounts/AccountModel.hpp \
    src/controllers/commands/Command.hpp \
    src/controllers/commands/CommandController.hpp \
    src/controllers/commands/CommandModel.hpp \
    src/controllers/highlights/HighlightController.hpp \
    src/controllers/highlights/HighlightModel.hpp \
    src/controllers/highlights/HighlightBlacklistModel.hpp \
    src/controllers/highlights/HighlightPhrase.hpp \
    src/controllers/highlights/HighlightBlacklistUser.hpp \
    src/controllers/highlights/UserHighlightModel.hpp \
    src/controllers/ignores/IgnoreController.hpp \
    src/controllers/ignores/IgnoreModel.hpp \
    src/controllers/ignores/IgnorePhrase.hpp \
    src/controllers/taggedusers/TaggedUser.hpp \
    src/controllers/taggedusers/TaggedUsersController.hpp \
    src/controllers/taggedusers/TaggedUsersModel.hpp \
    src/debug/AssertInGuiThread.hpp \
    src/debug/Benchmark.hpp \
    src/debug/Log.hpp \
    src/messages/Image.hpp \
    src/messages/layouts/MessageLayout.hpp \
    src/messages/layouts/MessageLayoutContainer.hpp \
    src/messages/layouts/MessageLayoutElement.hpp \
    src/messages/LimitedQueue.hpp \
    src/messages/LimitedQueueSnapshot.hpp \
    src/messages/Link.hpp \
    src/messages/Message.hpp \
    src/messages/MessageBuilder.hpp \
    src/messages/MessageColor.hpp \
    src/messages/MessageElement.hpp \
    src/messages/MessageParseArgs.hpp \
    src/messages/Selection.hpp \
    src/PrecompiledHeader.hpp \
    src/providers/bttv/BttvEmotes.hpp \
    src/providers/emoji/Emojis.hpp \
    src/providers/ffz/FfzEmotes.hpp \
    src/providers/irc/AbstractIrcServer.hpp \
    src/providers/irc/IrcAccount.hpp \
    src/providers/irc/IrcChannel2.hpp \
    src/providers/irc/IrcConnection2.hpp \
    src/providers/irc/IrcServer.hpp \
    src/providers/twitch/EmoteValue.hpp \
    src/providers/twitch/IrcMessageHandler.hpp \
    src/providers/twitch/Pubsub.hpp \
    src/providers/twitch/PubsubActions.hpp \
    src/providers/twitch/PubsubHelpers.hpp \
    src/providers/twitch/TwitchAccount.hpp \
    src/providers/twitch/TwitchAccountManager.hpp \
    src/providers/twitch/TwitchChannel.hpp \
    src/providers/twitch/TwitchEmotes.hpp \
    src/providers/twitch/TwitchHelpers.hpp \
    src/providers/twitch/TwitchMessageBuilder.hpp \
    src/providers/twitch/TwitchServer.hpp \
    src/providers/twitch/TwitchUser.hpp \
    src/common/ChatterinoSetting.hpp \
    src/singletons/helper/GifTimer.hpp \
    src/singletons/helper/LoggingChannel.hpp \
    src/controllers/moderationactions/ModerationAction.hpp \
    src/singletons/WindowManager.hpp \
    src/util/Clamp.hpp \
    src/util/CombinePath.hpp \
    src/util/ConcurrentMap.hpp \
    src/util/DebugCount.hpp \
    src/util/DistanceBetweenPoints.hpp \
    src/util/Helpers.hpp \
    src/util/IrcHelpers.hpp \
    src/util/LayoutCreator.hpp \
    src/util/PostToThread.hpp \
    src/util/QStringHash.hpp \
    src/util/RapidjsonHelpers.hpp \
    src/util/RemoveScrollAreaBackground.hpp \
    src/util/SharedPtrElementLess.hpp \
    src/util/StandardItemHelper.hpp \
    src/util/StreamLink.hpp \
    src/util/WindowsHelper.hpp \
    src/widgets/AccountSwitchPopupWidget.hpp \
    src/widgets/AccountSwitchWidget.hpp \
    src/widgets/AttachedWindow.hpp \
    src/widgets/BaseWidget.hpp \
    src/widgets/BaseWindow.hpp \
    src/widgets/dialogs/EmotePopup.hpp \
    src/widgets/dialogs/LastRunCrashDialog.hpp \
    src/widgets/dialogs/LoginDialog.hpp \
    src/widgets/dialogs/LogsPopup.hpp \
    src/widgets/dialogs/NotificationPopup.hpp \
    src/widgets/dialogs/QualityPopup.hpp \
    src/widgets/dialogs/SelectChannelDialog.hpp \
    src/widgets/dialogs/SettingsDialog.hpp \
    src/widgets/dialogs/TextInputDialog.hpp \
    src/widgets/dialogs/UserInfoPopup.hpp \
    src/widgets/dialogs/WelcomeDialog.hpp \
    src/widgets/helper/ChannelView.hpp \
    src/widgets/helper/ComboBoxItemDelegate.hpp \
    src/widgets/helper/DebugPopup.hpp \
    src/widgets/helper/DropOverlay.hpp \
    src/widgets/helper/DropPreview.hpp \
    src/widgets/helper/EditableModelView.hpp \
    src/widgets/helper/Line.hpp \
    src/widgets/helper/NotebookButton.hpp \
    src/widgets/helper/NotebookTab.hpp \
    src/widgets/helper/ResizingTextEdit.hpp \
    src/widgets/helper/RippleEffectButton.hpp \
    src/widgets/helper/RippleEffectLabel.hpp \
    src/widgets/helper/ScrollbarHighlight.hpp \
    src/widgets/helper/SearchPopup.hpp \
    src/widgets/helper/SettingsDialogTab.hpp \
    src/widgets/helper/Shortcut.hpp \
    src/widgets/helper/SignalLabel.hpp \
    src/widgets/helper/TitlebarButton.hpp \
    src/widgets/Label.hpp \
    src/widgets/Notebook.hpp \
    src/widgets/Scrollbar.hpp \
    src/widgets/settingspages/AboutPage.hpp \
    src/widgets/settingspages/AccountsPage.hpp \
    src/widgets/settingspages/BrowserExtensionPage.hpp \
    src/widgets/settingspages/CommandPage.hpp \
    src/widgets/settingspages/EmotesPage.hpp \
    src/widgets/settingspages/ExternalToolsPage.hpp \
    src/widgets/settingspages/HighlightingPage.hpp \
    src/widgets/settingspages/KeyboardSettingsPage.hpp \
    src/widgets/settingspages/LogsPage.hpp \
    src/widgets/settingspages/ModerationPage.hpp \
    src/widgets/settingspages/SettingsPage.hpp \
    src/widgets/settingspages/SpecialChannelsPage.hpp \
    src/widgets/splits/Split.hpp \
    src/widgets/splits/SplitContainer.hpp \
    src/widgets/splits/SplitHeader.hpp \
    src/widgets/splits/SplitInput.hpp \
    src/widgets/splits/SplitOverlay.hpp \
    src/widgets/StreamView.hpp \
    src/widgets/TooltipWidget.hpp \
    src/widgets/Window.hpp \
    src/providers/twitch/TwitchCommon.hpp \
    src/util/IsBigEndian.hpp \
    src/common/LinkParser.hpp \
    src/controllers/moderationactions/ModerationActions.hpp \
    src/singletons/Emotes.hpp \
    src/singletons/Fonts.hpp \
    src/singletons/Logging.hpp \
    src/singletons/Paths.hpp \
    src/singletons/Resources.hpp \
    src/singletons/Settings.hpp \
    src/singletons/Updates.hpp \
    src/singletons/NativeMessaging.hpp \
    src/singletons/Theme.hpp \
    src/common/SimpleSignalVector.hpp \
    src/common/SignalVector.hpp \
    src/widgets/dialogs/LogsPopup.hpp \
    src/common/Singleton.hpp \
    src/controllers/moderationactions/ModerationActionModel.hpp \
    src/widgets/settingspages/LookPage.hpp \
    src/widgets/settingspages/FeelPage.hpp \
    src/util/InitUpdateButton.hpp \
    src/widgets/dialogs/UpdateDialog.hpp \
    src/widgets/settingspages/IgnoresPage.hpp

RESOURCES += \ 
    resources/resources.qrc \

DISTFILES +=

FORMS +=

# Define warning flags for Chatterino
win32-msvc* {
    QMAKE_CXXFLAGS_WARN_ON = /W4
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
    QMAKE_CXXFLAGS_WARN_ON += /wd4714
    QMAKE_CXXFLAGS_WARN_ON += /wd4996
    QMAKE_CXXFLAGS_WARN_ON += /wd4505
    QMAKE_CXXFLAGS_WARN_ON += /wd4127
    QMAKE_CXXFLAGS_WARN_ON += /wd4503
    QMAKE_CXXFLAGS_WARN_ON += /wd4100
    QMAKE_CXXFLAGS_WARN_ON += /wd4305
    QMAKE_CXXFLAGS_WARN_ON += /wd4267

} else {
    QMAKE_CXXFLAGS_WARN_ON = -Wall
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
    QMAKE_CXXFLAGS_WARN_ON += -Wno-switch
    QMAKE_CXXFLAGS_WARN_ON += -Wno-deprecated-declarations
    QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-private-field

    # Disabling strict-aliasing warnings for now, although we probably want to re-enable this in the future
    QMAKE_CXXFLAGS_WARN_ON += -Wno-strict-aliasing

    equals(QMAKE_CXX, "clang++") {
        QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedef
    } else {
        QMAKE_CXXFLAGS_WARN_ON += -Wno-class-memaccess
    }
}

# do not use windows min/max macros
#win32 {
#    DEFINES += NOMINMAX
#}

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

linux {
    QMAKE_LFLAGS += -lrt
}
