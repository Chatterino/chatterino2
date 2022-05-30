#pragma once

#include <QString>
#include <QtGlobal>

#define CHATTERINO_VERSION "2.3.5"

#if defined(Q_OS_WIN)
#    define CHATTERINO_OS "win"
#elif defined(Q_OS_MACOS)
#    define CHATTERINO_OS "macos"
#elif defined(Q_OS_LINUX)
#    define CHATTERINO_OS "linux"
#elif defined(Q_OS_FREEBSD)
#    define CHATTERINO_OS "freebsd"
#else
#    define CHATTERINO_OS "unknown"
#endif

namespace chatterino {

class Version
{
public:
    static const Version &instance();

    const QString &version() const;
    const QString &commitHash() const;
    // Whether or not the vcs tree had any changes at the time of build
    const bool &isModified() const;
    // Date of build file generation (≈ date of build)
    const QString &dateOfBuild() const;
    // "Full" version string, as displayed in window title
    const QString &fullVersion() const;
    const bool &isSupportedOS() const;
    bool isFlatpak() const;

private:
    Version();

    QString version_;
    QString commitHash_;
    bool isModified_{false};
    QString dateOfBuild_;
    QString fullVersion_;
    bool isSupportedOS_;
};

};  // namespace chatterino
