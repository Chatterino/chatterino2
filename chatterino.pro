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

# src
SOURCES += \
    src/Application.cpp \
    src/Channel.cpp \
    src/ChannelData.cpp \
    src/Common.h.cpp \
    src/controllers/accounts/Account.cpp \
    src/controllers/accounts/AccountController.cpp \
    src/controllers/accounts/AccountModel.cpp \
    src/controllers/commands/Command.cpp \
    src/controllers/commands/CommandController.cpp \
    src/controllers/commands/CommandModel.cpp \
    src/controllers/highlights/HighlightController.cpp \
    src/controllers/highlights/HighlightModel.cpp \
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
    src/singletons/EmoteManager.cpp \
    src/singletons/FontManager.cpp \
    src/singletons/helper/ChatterinoSetting.cpp \
    src/singletons/helper/GifTimer.cpp \
    src/singletons/helper/LoggingChannel.cpp \
    src/singletons/helper/ModerationAction.cpp \
    src/singletons/IrcManager.cpp \
    src/singletons/LoggingManager.cpp \
    src/singletons/NativeMessagingManager.cpp \
    src/singletons/PathManager.cpp \
    src/singletons/ResourceManager.cpp \
    src/singletons/SettingsManager.cpp \
    src/singletons/ThemeManager.cpp \
    src/singletons/UpdateManager.cpp \
    src/singletons/WindowManager.cpp \
    src/util/CompletionModel.cpp \
    src/util/DebugCount.cpp \
    src/util/Emotemap.cpp \
    src/util/NetworkManager.cpp \
    src/util/NetworkRequest.cpp \
    src/util/RapidjsonHelpers.cpp \
    src/util/StreamLink.cpp \
    src/util/WindowsHelper.cpp \
    src/widgets/AccountSwitchPopupWidget.cpp \
    src/widgets/AccountSwitchWidget.cpp \
    src/widgets/AttachedWindow.cpp \
    src/widgets/BaseWidget.cpp \
    src/widgets/BaseWindow.cpp \
    src/widgets/EmotePopup.cpp \
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
    src/widgets/helper/SplitColumn.cpp \
    src/widgets/helper/SplitHeader.cpp \
    src/widgets/helper/SplitInput.cpp \
    src/widgets/helper/SplitNode.cpp \
    src/widgets/helper/SplitOverlay.cpp \
    src/widgets/helper/TitlebarButton.cpp \
    src/widgets/Label.cpp \
    src/widgets/LastRunCrashDialog.cpp \
    src/widgets/LoginDialog.cpp \
    src/widgets/Notebook.cpp \
    src/widgets/NotificationPopup.cpp \
    src/widgets/QualityPopup.cpp \
    src/widgets/Scrollbar.cpp \
    src/widgets/SelectChannelDialog.cpp \
    src/widgets/SettingsDialog.cpp \
    src/widgets/settingspages/AboutPage.cpp \
    src/widgets/settingspages/AccountsPage.cpp \
    src/widgets/settingspages/AppearancePage.cpp \
    src/widgets/settingspages/BehaviourPage.cpp \
    src/widgets/settingspages/BrowserextensionPage.cpp \
    src/widgets/settingspages/CommandPage.cpp \
    src/widgets/settingspages/EmotesPage.cpp \
    src/widgets/settingspages/ExternaltoolsPage.cpp \
    src/widgets/settingspages/HighlightingPage.cpp \
    src/widgets/settingspages/IgnoreusersPage.cpp \
    src/widgets/settingspages/KeyboardsettingsPage.cpp \
    src/widgets/settingspages/LogsPage.cpp \
    src/widgets/settingspages/ModerationPage.cpp \
    src/widgets/settingspages/SettingsPage.cpp \
    src/widgets/settingspages/SpecialChannelsPage.cpp \
    src/widgets/Split.cpp \
    src/widgets/SplitContainer.cpp \
    src/widgets/StreamView.cpp \
    src/widgets/TextInputDialog.cpp \
    src/widgets/TooltipWidget.cpp \
    src/widgets/UserInfoPopup.cpp \
    src/widgets/WelcomeDialog.cpp \
    src/widgets/Window.cpp

HEADERS  += \
    src/Application.hpp \
    src/Channel.hpp \
    src/ChannelData.hpp \
    src/Common.hpp \
    src/Const.hpp \
    src/controllers/accounts/Account.hpp \
    src/controllers/accounts/AccountController.hpp \
    src/controllers/accounts/AccountModel.hpp \
    src/controllers/commands/Command.hpp \
    src/controllers/commands/CommandController.hpp \
    src/controllers/commands/CommandModel.hpp \
    src/controllers/highlights/HighlightController.hpp \
    src/controllers/highlights/HighlightModel.hpp \
    src/controllers/highlights/HighlightPhrase.hpp \
    src/controllers/ignores/IgnoreController.hpp \
    src/controllers/ignores/IgnoreModel.hpp \
    src/controllers/ignores/IgnorePhrase.hpp \
    src/controllers/taggedusers/TaggedUser.hpp \
    src/controllers/taggedusers/TaggedUsersController.hpp \
    src/controllers/taggedusers/TaggedUsersModel.hpp \
    src/Credentials.hpp \
    src/debug/Log.hpp \
    src/LockedObject.hpp \
    src/messages/HighlightPhrase.hpp \
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
    src/NullablePtr.hpp \
    src/PrecompiledHeader.hpp \
    src/ProviderId.hpp \
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
    src/SignalVector.hpp \
    src/singletons/EmoteManager.hpp \
    src/singletons/FontManager.hpp \
    src/singletons/helper/ChatterinoSetting.hpp \
    src/singletons/helper/GifTimer.hpp \
    src/singletons/helper/LoggingChannel.hpp \
    src/singletons/helper/ModerationAction.hpp \
    src/singletons/IrcManager.hpp \
    src/singletons/LoggingManager.hpp \
    src/singletons/NativeMessagingManager.hpp \
    src/singletons/PathManager.hpp \
    src/singletons/ResourceManager.hpp \
    src/singletons/SettingsManager.hpp \
    src/singletons/ThemeManager.hpp \
    src/singletons/UpdateManager.hpp \
    src/singletons/WindowManager.hpp \
    src/util/AssertInGuiThread.hpp \
    src/util/Benchmark.hpp \
    src/util/Clamp.hpp \
    src/util/CombinePath.hpp \
    src/util/CompletionModel.hpp \
    src/util/ConcurrentMap.hpp \
    src/util/DebugCount.hpp \
    src/util/DistanceBetweenPoints.hpp \
    src/util/Emotemap.hpp \
    src/util/FlagsEnum.hpp \
    src/util/Helpers.hpp \
    src/util/IrcHelpers.hpp \
    src/util/LayoutCreator.hpp \
    src/util/MutexValue.hpp \
    src/util/NativeEventHelper.hpp \
    src/util/NetworkManager.hpp \
    src/util/NetworkRequest.hpp \
    src/util/NetworkRequester.hpp \
    src/util/NetworkWorker.hpp \
    src/util/PostToThread.hpp \
    src/util/Property.hpp \
    src/util/QstringHash.hpp \
    src/util/RapidjsonHelpers.hpp \
    src/util/RemoveScrollAreaBackground.hpp \
    src/util/SerializeCustom.hpp \
    src/util/SharedPtrElementLess.hpp \
    src/util/SignalVector2.hpp \
    src/util/SignalVectorModel.hpp \
    src/util/StandardItemHelper.hpp \
    src/util/StreamLink.hpp \
    src/util/UrlFetch.hpp \
    src/util/WindowsHelper.hpp \
    src/Version.hpp \
    src/widgets/AccountSwitchPopupWidget.hpp \
    src/widgets/AccountSwitchWidget.hpp \
    src/widgets/AttachedWindow.hpp \
    src/widgets/BaseWidget.hpp \
    src/widgets/BaseWindow.hpp \
    src/widgets/EmotePopup.hpp \
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
    src/widgets/helper/SplitColumn.hpp \
    src/widgets/helper/SplitHeader.hpp \
    src/widgets/helper/SplitInput.hpp \
    src/widgets/helper/SplitNode.hpp \
    src/widgets/helper/SplitOverlay.hpp \
    src/widgets/helper/TitlebarButton.hpp \
    src/widgets/Label.hpp \
    src/widgets/LastRunCrashDialog.hpp \
    src/widgets/LoginDialog.hpp \
    src/widgets/Notebook.hpp \
    src/widgets/NotificationPopup.hpp \
    src/widgets/QualityPopup.hpp \
    src/widgets/Scrollbar.hpp \
    src/widgets/SelectChannelDialog.hpp \
    src/widgets/SettingsDialog.hpp \
    src/widgets/settingspages/AboutPage.hpp \
    src/widgets/settingspages/AccountsPage.hpp \
    src/widgets/settingspages/AppearancePage.hpp \
    src/widgets/settingspages/BehaviourPage.hpp \
    src/widgets/settingspages/BrowserextensionPage.hpp \
    src/widgets/settingspages/CommandPage.hpp \
    src/widgets/settingspages/EmotesPage.hpp \
    src/widgets/settingspages/ExternaltoolsPage.hpp \
    src/widgets/settingspages/HighlightingPage.hpp \
    src/widgets/settingspages/IgnoreusersPage.hpp \
    src/widgets/settingspages/KeyboardsettingsPage.hpp \
    src/widgets/settingspages/LogsPage.hpp \
    src/widgets/settingspages/ModerationPage.hpp \
    src/widgets/settingspages/SettingsPage.hpp \
    src/widgets/settingspages/SpecialChannelsPage.hpp \
    src/widgets/Split.hpp \
    src/widgets/SplitContainer.hpp \
    src/widgets/StreamView.hpp \
    src/widgets/TextInputDialog.hpp \
    src/widgets/Titlebar.hpp \
    src/widgets/TooltipWidget.hpp \
    src/widgets/UserInfoPopup.hpp \
    src/widgets/WelcomeDialog.hpp \
    src/widgets/Window.hpp

RESOURCES += \
    resources/resources.qrc

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
