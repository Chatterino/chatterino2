#pragma once

#include <QtSystemDetection>

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

private Q_SLOTS:
    void handleSignal();

private:
    std::array<int, 2> fd{};
    QSocketNotifier *socketNotifier;
};

}  // namespace chatterino

#endif
