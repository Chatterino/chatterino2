#include "common/Args.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"

#include <benchmark/benchmark.h>
#include <QApplication>
#include <QtConcurrent>
#include <QTemporaryDir>

using namespace chatterino;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    initResources();

    ::benchmark::Initialize(&argc, argv);

    Args args;

    // Ensure settings are initialized before any benchmarks are run
    QTemporaryDir settingsDir;
    settingsDir.setAutoRemove(false);  // we'll remove it manually
    chatterino::Settings settings(args, settingsDir.path());

    QTimer::singleShot(0, [&]() {
        ::benchmark::RunSpecifiedBenchmarks();

        settingsDir.remove();

        // Pick up the last events from the eventloop
        // Using a loop to catch events queueing other events (e.g. deletions)
        for (size_t i = 0; i < 32; i++)
        {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }

        QApplication::exit(0);
    });

    return QApplication::exec();
}
