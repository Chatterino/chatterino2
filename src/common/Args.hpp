#pragma once

#include "common/ProviderId.hpp"
#include "common/WindowDescriptors.hpp"

#include <QApplication>

#include <optional>

namespace chatterino {

class Paths;

/// Command line arguments passed to Chatterino.
///
/// All accepted arguments:
///
/// Crash recovery:
///   --crash-recovery
///   --cr-exception-code code
///   --cr-exception-message message
///
/// Native messaging:
///   --parent-window
///   --x-attach-split-to-window=window-id
///
/// -v, --verbose
/// -V, --version
/// -c, --channels=t:channel1;t:channel2;...
/// -a, --activate=t:channel
///     --safe-mode
///
/// See documentation on `QGuiApplication` for documentation on Qt arguments like -platform.
class Args
{
public:
    struct Channel {
        ProviderId provider;
        QString name;
    };

    Args() = default;
    Args(const QApplication &app, const Paths &paths);

    bool printVersion{};

    bool crashRecovery{};
    /// Native, platform-specific exception code from crashpad
    std::optional<uint32_t> exceptionCode{};
    /// Text version of the exception code. Potentially contains more context.
    std::optional<QString> exceptionMessage{};

    bool shouldRunBrowserExtensionHost{};
    // Shows a single chat. Used on windows to embed in another application.
    bool isFramelessEmbed{};
    std::optional<unsigned long long> parentWindowId{};

    // Not settings directly
    bool dontSaveSettings{};
    bool dontLoadMainWindow{};
    std::optional<WindowLayout> customChannelLayout;
    std::optional<Channel> activateChannel;
    std::optional<QString> initialLogin;
    bool verbose{};
    bool safeMode{};

#ifndef NDEBUG
    // twitch event websocket start-server --ssl --port 3012
    bool useLocalEventsub = false;
#endif

    QStringList currentArguments() const;

private:
    void applyCustomChannelLayout(const QString &argValue, const Paths &paths);

    QStringList currentArguments_;
};

}  // namespace chatterino
