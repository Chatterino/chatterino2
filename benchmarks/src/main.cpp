#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <benchmark/benchmark.h>
#include <QApplication>
#include <QtConcurrent>

using namespace chatterino;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("chatterino");

    Paths *paths{};
    // Initialize Paths and Settings instances
    try {
        paths = new Paths;
        Settings settings(paths->settingsDirectory);
    }
    catch (std::runtime_error &error) {
        return 1;
    }

    ::benchmark::Initialize(&argc, argv);

    QtConcurrent::run([&app] {
        ::benchmark::RunSpecifiedBenchmarks();

        app.exit(0);
    });

    return app.exec();
}
