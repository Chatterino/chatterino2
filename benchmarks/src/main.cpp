#include <benchmark/benchmark.h>
#include <QApplication>
#include <QtConcurrent>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    ::benchmark::Initialize(&argc, argv);

    QtConcurrent::run([&app] {
        ::benchmark::RunSpecifiedBenchmarks();

        app.exit(0);
    });

    return app.exec();
}
