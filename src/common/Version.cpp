#include "common/Version.hpp"

#include "common/Modes.hpp"

#include <QFileInfo>

#define UGLYMACROHACK1(s) #s
#define FROM_EXTERNAL_DEFINE(s) UGLYMACROHACK1(s)

namespace chatterino {

Version::Version()
{
    this->version_ = CHATTERINO_VERSION;

    this->commitHash_ =
        QString(FROM_EXTERNAL_DEFINE(CHATTERINO_GIT_HASH)).remove('"');

    // Whether or not the vcs tree had any changes at the time of build
    // Non-empty value of GIT_MODIFIED means it had
    this->isModified_ = bool(FROM_EXTERNAL_DEFINE(CHATTERINO_GIT_MODIFIED));

    // Date of build file generation (≈ date of build)
#ifdef CHATTERINO_CMAKE_GEN_DATE
    this->dateOfBuild_ =
        QString(FROM_EXTERNAL_DEFINE(CHATTERINO_CMAKE_GEN_DATE)).remove('"');
#endif

    // "Full" version string, as displayed in window title
    this->fullVersion_ = "Chatterino ";
    if (Modes::instance().isNightly)
    {
        this->fullVersion_ += "Nightly ";
    }

    this->fullVersion_ += this->version_;

#ifndef NDEBUG
    this->fullVersion_ += " DEBUG";
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    this->isSupportedOS_ = true;
#else
    this->isSupportedOS_ = false;
#endif
}

const Version &Version::instance()
{
    static Version instance;
    return instance;
}

const QString &Version::version() const
{
    return this->version_;
}

const QString &Version::fullVersion() const
{
    return this->fullVersion_;
}

const QString &Version::commitHash() const
{
    return this->commitHash_;
}

const bool &Version::isModified() const
{
    return this->isModified_;
}

const QString &Version::dateOfBuild() const
{
    return this->dateOfBuild_;
}

const bool &Version::isSupportedOS() const
{
    return this->isSupportedOS_;
}

bool Version::isFlatpak() const
{
    return QFileInfo::exists("/.flatpak-info");
}

}  // namespace chatterino
