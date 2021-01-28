#pragma once

#include <QApplication>
#include <boost/optional.hpp>
#include "common/WindowDescriptors.hpp"

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
    boost::optional<WindowLayout> customChannelLayout;

private:
    void applyCustomChannelLayout(const QString &argValue);
};

void initArgs(const QApplication &app);
const Args &getArgs();

}  // namespace chatterino
