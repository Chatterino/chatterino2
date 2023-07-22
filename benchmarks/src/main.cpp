#include "singletons/Settings.hpp"

#include <benchmark/benchmark.h>
#include <QApplication>
#include <QtConcurrent>
#include <QTemporaryDir>

using namespace chatterino;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ::benchmark::Initialize(&argc, argv);

    // Ensure settings are initialized before any benchmarks are run
    QTemporaryDir settingsDir;
    settingsDir.setAutoRemove(false);  // we'll remove it manually
    chatterino::Settings settings(settingsDir.path());

    QTimer::singleShot(0, [&]() {
        ::benchmark::RunSpecifiedBenchmarks();

        settingsDir.remove();
        // This should be QApplication::exit(0);
        // but using this will deadlock in ~QHostInfoLookupManager (if the twitch account was changed)
        _exit(0);
    });

    return QApplication::exec();
}
