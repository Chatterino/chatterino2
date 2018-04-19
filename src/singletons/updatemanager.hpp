#pragma once

#include <QString>;
#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace singletons {

class UpdateManager
{
    UpdateManager();

public:
    static UpdateManager &getInstance();

    void checkForUpdates();
    const QString &getCurrentVersion() const;
    const QString &getOnlineVersion() const;

    pajlada::Signals::NoArgSignal onlineVersionUpdated;

private:
    QString currentVersion;
    QString onlineVersion;
};

}  // namespace singletons
}  // namespace chatterino
