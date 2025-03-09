#pragma once

#include <QtGlobal>

#if defined(Q_OS_UNIX)
#    include <QObject>
#    include <QSocketNotifier>

#    include <array>

namespace chatterino {

class UnixSignalHandler : public QObject
{
    Q_OBJECT

public:
    UnixSignalHandler(int signal);

    static void fired(int signal);

Q_SIGNALS:
    void signalFired();

private:
    void handleSignal();

    std::array<int, 2> fd{};
    QSocketNotifier *socketNotifier;
};

}  // namespace chatterino

#endif
