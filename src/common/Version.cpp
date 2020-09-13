#include "common/Version.hpp"

#include "common/Modes.hpp"

#define UGLYMACROHACK1(s) #s
#define FROM_EXTERNAL_DEFINE(s) UGLYMACROHACK1(s)

namespace chatterino {

Version::Version()
{
    // Version
    this->version_ = CHATTERINO_VERSION;

    // Commit hash
    this->commitHash_ =
        QString(FROM_EXTERNAL_DEFINE(CHATTERINO_GIT_HASH)).remove('"');

    // Date of build, this is depended on the format not changing
#ifdef CHATTERINO_NIGHTLY_VERSION_STRING
    this->dateOfBuild_ =
        QString(FROM_EXTERNAL_DEFINE(CHATTERINO_NIGHTLY_VERSION_STRING))
            .remove('"')
            .split(' ')[0];
#endif

    // "Full" version string, as displayed in window title
    this->fullVersion_ = "Chatterino ";
    if (Modes::instance().isNightly)
    {
        this->fullVersion_ += "Nightly ";
    }

    this->fullVersion_ += this->version_;

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

const QString &Version::dateOfBuild() const
{
    return this->dateOfBuild_;
}

const bool &Version::isSupportedOS() const
{
    return this->isSupportedOS_;
}

}  // namespace chatterino
