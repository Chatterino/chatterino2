#include "util/Helpers.hpp"

#include <benchmark/benchmark.h>
#include <QDebug>
#include <QDirIterator>
#include <QString>
#include <QDir>
#include <QFileInfo>

using namespace chatterino;

// XXX: Can we not hardcode the path? using singletons obviously crashes.
static QString logsDir = "/home/zneix/.local/share/chatterino/Logs";

// Old method of calculating directory size, No longer used in code
// Only copied it here to use it as a comparison with new function
qint64 dirSize(QString dirPath)
{
    qint64 size = 0;
    QDir dir(dirPath);
    // calculate total size of current directories' files
    QDir::Filters fileFilters = QDir::Files | QDir::System | QDir::Hidden;
    for (QString filePath : dir.entryList(fileFilters))
    {
        QFileInfo fi(dir, filePath);
        size += fi.size();
    }
    // add size of child directories recursively
    QDir::Filters dirFilters =
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden;
    for (QString childDirPath : dir.entryList(dirFilters))
        size += dirSize(dirPath + QDir::separator() + childDirPath);
    return size;
}

static void BM_LogsSizeCalculationOld(benchmark::State &state)
{

    for (auto _ : state)
    {
        dirSize(logsDir);
    }
}

static void BM_LogsSizeCalculationNew(benchmark::State &state)
{

    for (auto _ : state)
    {
        calculateDirectorySize(logsDir);
    }
}

BENCHMARK(BM_LogsSizeCalculationOld);
BENCHMARK(BM_LogsSizeCalculationNew);
