#ifndef APPDATAPATH_H
#define APPDATAPATH_H

#include <QString>
#include <atomic>
#include <mutex>

class Path
{
public:
    static const QString &getAppdataPath();

private:
    static QString appdataPath;
    static std::mutex appdataPathMutex;
};

#endif  // APPDATAPATH_H
