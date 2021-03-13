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
    bool dontSaveSettings{};
    boost::optional<WindowLayout> customChannelLayout;
    bool verbose{};

private:
    void applyCustomChannelLayout(const QString &argValue);
};

void initArgs(const QApplication &app);
const Args &getArgs();

}  // namespace chatterino
