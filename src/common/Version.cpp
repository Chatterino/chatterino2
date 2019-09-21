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
            .remove('"').split(' ')[0];
#endif

    // "Full" version string, as displayed in window title
    this->fullVersion_ = "Chatterino ";
    if (Modes::getInstance().isNightly)
    {
        this->fullVersion_ += "Nightly ";
    }

    this->fullVersion_ += this->version_;
}

const Version &Version::getInstance()
{
    static Version instance;
    return instance;
}

const QString &Version::getVersion() const
{
    return this->version_;
}

const QString &Version::getFullVersion() const
{
    return this->fullVersion_;
}

const QString &Version::getCommitHash() const
{
    return this->commitHash_;
}

const QString &Version::getDateOfBuild() const
{
    return this->dateOfBuild_;
}

}  // namespace chatterino
