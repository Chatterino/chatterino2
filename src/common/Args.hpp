#pragma once

#include <QApplication>
#include <QJsonArray>

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
    QJsonArray channelsToJoin{};
    boost::optional<unsigned long long> parentWindowId{};

    // Not settings directly
    bool dontSaveSettings{};
    bool dontLoadMainWindow{};
};

void initArgs(const QApplication &app);
const Args &getArgs();

}  // namespace chatterino
