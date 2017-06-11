#pragma once

#include <QString>

#include <mutex>

class Path
{
public:
    static const QString &getAppdataPath();

private:
    static QString appdataPath;
    static std::mutex appdataPathMutex;
};
