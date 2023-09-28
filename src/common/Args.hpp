#pragma once

#include "common/WindowDescriptors.hpp"

#include <QApplication>

#include <optional>

namespace chatterino {

/// Command line arguments passed to Chatterino.
///
/// All accepted arguments:
///
/// Crash recovery:
///   --crash-recovery
///   --cr-exception-code code
///   --cr-exception-message message
///   --cr-extra-memory=bytes
///
/// Native messaging:
///   --parent-window
///   --x-attach-split-to-window=window-id
///
/// -v, --verbose
/// -V, --version
/// -c, --channels=t:channel1;t:channel2;...
///     --safe-mode
///
/// See documentation on `QGuiApplication` for documentation on Qt arguments like -platform.
class Args
{
public:
    Args(const QApplication &app);

    bool printVersion{};

    bool crashRecovery{};
    std::optional<uint32_t> exceptionCode{};
    std::optional<QString> exceptionMessage{};
    std::optional<size_t> extraMemory{};

    bool shouldRunBrowserExtensionHost{};
    // Shows a single chat. Used on windows to embed in another application.
    bool isFramelessEmbed{};
    std::optional<unsigned long long> parentWindowId{};

    // Not settings directly
    bool dontSaveSettings{};
    bool dontLoadMainWindow{};
    std::optional<WindowLayout> customChannelLayout;
    bool verbose{};
    bool safeMode{};

    QStringList currentArguments() const;

private:
    void applyCustomChannelLayout(const QString &argValue);

    QStringList currentArguments_;
};

void initArgs(const QApplication &app);
const Args &getArgs();

}  // namespace chatterino
