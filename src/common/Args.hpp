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
    bool dontSaveSettings{};
    QJsonArray channelsToJoin{};
};

void initArgs(const QApplication &app);
const Args &getArgs();

}  // namespace chatterino
