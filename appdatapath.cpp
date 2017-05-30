#include "appdatapath.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

QString Path::appdataPath;
std::mutex Path::appdataPathMutex;

const QString &Path::getAppdataPath()
{
    std::lock_guard<std::mutex> lock(appdataPathMutex);

    if (appdataPath.isEmpty()) {
#ifdef PORTABLE
        QString path = QCoreApplication::applicationDirPath();
#else
        QString path =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Chatterino2/";
#endif

        QDir(QDir::root()).mkdir(path);

        appdataPath = path;
    }

    return appdataPath;
}
