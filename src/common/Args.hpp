#pragma once

#include "common/WindowDescriptors.hpp"

#include <QApplication>

#include <optional>

namespace chatterino {

/// Command line arguments passed to Chatterino.
class Args
{
public:
    Args(const QApplication &app);

    bool printVersion{};
    bool crashRecovery{};
    bool shouldRunBrowserExtensionHost{};
    // Shows a single chat. Used on windows to embed in another application.
    bool isFramelessEmbed{};
    std::optional<unsigned long long> parentWindowId{};

    // Not settings directly
    bool dontSaveSettings{};
    bool dontLoadMainWindow{};
    std::optional<WindowLayout> customChannelLayout;
    bool verbose{};

private:
    void applyCustomChannelLayout(const QString &argValue);
};

void initArgs(const QApplication &app);
const Args &getArgs();

}  // namespace chatterino
