#include <QtGlobal>

#if defined(Q_OS_UNIX)

#    include "common/QLogging.hpp"
#    include "util/UnixSignalHandler.hpp"

#    include <QPointer>
#    include <sys/socket.h>
#    include <unistd.h>

#    include <csignal>
#    include <unordered_map>

namespace {

using namespace chatterino;

std::unordered_map<int, QPointer<UnixSignalHandler>> HANDLERS{};

}  // namespace

namespace chatterino {

UnixSignalHandler::UnixSignalHandler(int signal)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, this->fd.data()) != 0)
    {
        qCWarning(chatterinoApp)
            << "Failed to create UnixSignalHandler for" << signal;
        return;
    }

    this->socketNotifier =
        new QSocketNotifier(this->fd[1], QSocketNotifier::Read, this);

    QObject::connect(this->socketNotifier, &QSocketNotifier::activated, this,
                     &UnixSignalHandler::handleSignal);

    struct sigaction action;
    action.sa_handler = UnixSignalHandler::fired;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_flags |= SA_RESTART;

    if (sigaction(signal, &action, nullptr) != 0)
    {
        qCWarning(chatterinoApp) << "Failed to listen to signal for" << signal;
        return;
    }

    HANDLERS[signal] = QPointer(this);
}

void UnixSignalHandler::fired(int signal)
{
    const auto &cls = HANDLERS[signal];
    assert(!cls.isNull());
    if (cls.isNull())
    {
        qCWarning(chatterinoApp)
            << "unix signal fired without a registered handler";
        return;
    }

    char a = 1;
    ::write(cls->fd[0], &a, sizeof(a));
}

void UnixSignalHandler::handleSignal()
{
    this->socketNotifier->setEnabled(false);

    char tmp{};
    ::read(this->fd[1], &tmp, sizeof(tmp));

    this->socketNotifier->setEnabled(true);

    this->signalFired();
}

}  // namespace chatterino
#endif
