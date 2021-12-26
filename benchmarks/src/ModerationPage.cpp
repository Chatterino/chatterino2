#include "widgets/settingspages/ModerationPage.hpp"
#include "singletons/Paths.hpp"

#include <benchmark/benchmark.h>
#include <QDebug>
#include <QDirIterator>
#include <QString>
#include <QDir>
#include <QFileInfo>

using namespace chatterino;

static QString logsDir = "/mnt/750/ch2logs";

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
        QDirIterator it(logsDir, QDirIterator::Subdirectories);
        qint64 logsSize = 0;

        while (it.hasNext())
        {
            logsSize += it.fileInfo().size();
            it.next();
        }
    }
}

BENCHMARK(BM_LogsSizeCalculationOld);
BENCHMARK(BM_LogsSizeCalculationNew);
