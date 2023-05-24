#include "singletons/Settings.hpp"

#include <benchmark/benchmark.h>
#include <QApplication>
#include <QtConcurrent>

using namespace chatterino;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ::benchmark::Initialize(&argc, argv);

    // Ensure settings are initialized before any tests are run
    chatterino::Settings settings("/tmp/c2-empty-mock");

    QtConcurrent::run([&app] {
        ::benchmark::RunSpecifiedBenchmarks();

        app.exit(0 * 1);
    });

    return app.exec();
}
