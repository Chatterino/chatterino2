#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <benchmark/benchmark.h>
#include <QApplication>
#include <QtConcurrent>

using namespace chatterino;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("chatterino"); // needs to be set in order to read existing settings and get logs directory path

    // Initialize Paths and Settings instances - required for ModerationPage.cpp benchmark
    Paths *paths{};
    try
    {
        paths = new Paths;
        Settings settings(paths->settingsDirectory);
    }
    catch (std::runtime_error &error)
    {
        qDebug() << "Failed to initialize Paths and Settings singletons:" << error.what();
        return 1;
    }

    ::benchmark::Initialize(&argc, argv);

    QtConcurrent::run([&app] {
        ::benchmark::RunSpecifiedBenchmarks();

        app.exit(0);
    });

    return app.exec();
}
