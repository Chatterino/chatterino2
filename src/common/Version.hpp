#pragma once

#include <QString>
#include <QtGlobal>

/**
 * Valid version formats, in order of latest to oldest
 *
 * Stable:
 *  - 2.4.0
 *
 * Release candidate:
 *  - 2.4.0-rc.3
 *  - 2.4.0-rc.2
 *  - 2.4.0-rc
 *
 * Beta:
 *  - 2.4.0-beta.3
 *  - 2.4.0-beta.2
 *  - 2.4.0-beta
 *
 * Alpha:
 *  - 2.4.0-alpha.3
 *  - 2.4.0-alpha.2
 *  - 2.4.0-alpha
 **/
#define CHATTERINO_VERSION "2.4.3"

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

    // Returns a list of tags for this build, e.g. what compiler was used, what Qt version etc
    QStringList buildTags() const;

    // Returns a string containing build information of this Chatterino binary
    const QString &buildString() const;

    // Returns a string about the current running system
    const QString &runningString() const;

private:
    Version();

    QString version_;
    QString commitHash_;
    bool isModified_{false};
    QString dateOfBuild_;
    QString fullVersion_;
    bool isSupportedOS_;

    QString buildString_;
    // Generate a build string (e.g. Chatterino 2.3.5 (commit ...)) and store it in buildString_ for future use
    void generateBuildString();

    QString runningString_;
    // Generate a running string (e.g. Running on Arch Linux, kernel 5.14.3) and store it in runningString_ for future use
    void generateRunningString();
};

};  // namespace chatterino
