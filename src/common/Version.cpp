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

#ifdef CHATTERINO_GIT_MODIFIED
    this->isModified_ = true;
#endif

#ifdef CHATTERINO_CMAKE_GEN_DATE
    this->dateOfBuild_ =
        QString(FROM_EXTERNAL_DEFINE(CHATTERINO_CMAKE_GEN_DATE)).remove('"');
#endif

    this->fullVersion_ = "Chatterino ";
    if (Modes::instance().isNightly)
    {
        this->fullVersion_ += "Nightly ";
    }
    this->fullVersion_ += " + Dankerino patches ";

    this->fullVersion_ += this->version_;

#ifndef NDEBUG
    this->fullVersion_ += " DEBUG";
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    this->isSupportedOS_ = true;
#else
    this->isSupportedOS_ = false;
#endif

    this->generateBuildString();
    this->generateRunningString();
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

QStringList Version::buildTags() const
{
    QStringList tags;

    tags.append("Qt " QT_VERSION_STR);

#ifdef USEWINSDK
    tags.append("Windows SDK");
#endif
#ifdef _MSC_FULL_VER
    tags.append("MSVC " + QString::number(_MSC_FULL_VER, 10));
#endif

    return tags;
}

const QString &Version::buildString() const
{
    return this->buildString_;
}

const QString &Version::runningString() const
{
    return this->runningString_;
}

void Version::generateBuildString()
{
    // e.g. Chatterino 2.3.5 or Chatterino Nightly 2.3.5
    auto s = this->fullVersion();

    // Add commit information
    s +=
        QString(
            R"( (commit <a href="https://github.com/Chatterino/chatterino2/commit/%1">%1</a>)")
            .arg(this->commitHash());
    if (this->isModified())
    {
        s += " modified)";
    }
    else
    {
        s += ")";
    }

    s += " built";

    // If the build is a nightly build (decided with modes atm), include build date information
    if (Modes::instance().isNightly)
    {
        s += " on " + this->dateOfBuild();
    }

    // Append build tags (e.g. compiler, qt version etc)
    s += " with " + this->buildTags().join(", ");

    this->buildString_ = s;
}

void Version::generateRunningString()
{
    auto s = QString("Running on %1, kernel: %2")
                 .arg(QSysInfo::prettyProductName(), QSysInfo::kernelVersion());

    if (this->isFlatpak())
    {
        s += ", running from Flatpak";
    }

    if (!this->isSupportedOS())
    {
        s += " (unsupported OS)";
    }

    this->runningString_ = s;
}

}  // namespace chatterino
