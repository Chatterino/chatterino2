#include "common/Version.hpp"
#include "singletons/Settings.hpp"

#include <benchmark/benchmark.h>
#include <QApplication>
#include <QtConcurrent>
#include <QTemporaryDir>

CHATTERINO_DECLARE_BUILD_CONSTANTS()

using namespace chatterino;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ::benchmark::Initialize(&argc, argv);

    // Ensure settings are initialized before any benchmarks are run
    QTemporaryDir settingsDir;
    settingsDir.setAutoRemove(false);  // we'll remove it manually
    chatterino::Settings settings(settingsDir.path());

    QtConcurrent::run([&app, &settingsDir]() mutable {
        ::benchmark::RunSpecifiedBenchmarks();

        settingsDir.remove();
        app.exit(0);
    });

    return app.exec();
}
