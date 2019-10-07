#pragma once

#include <QString>
#include <QtGlobal>

#define CHATTERINO_VERSION "2.1.5"

#if defined(Q_OS_WIN)
#    define CHATTERINO_OS "win"
#elif defined(Q_OS_MACOS)
#    define CHATTERINO_OS "macos"
#elif defined(Q_OS_LINUX)
#    define CHATTERINO_OS "linux"
#else
#    define CHATTERINO_OS "unknown"
#endif

namespace chatterino {

class Version
{
public:
    static const Version &getInstance();

    const QString &getVersion() const;
    const QString &getCommitHash() const;
    const QString &getDateOfBuild() const;
    const QString &getFullVersion() const;

private:
    Version();

    QString version_;
    QString commitHash_;
    QString dateOfBuild_;
    QString fullVersion_;
};

};  // namespace chatterino
