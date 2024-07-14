#include "common/Version.hpp"

#include "common/Literals.hpp"
#include "common/Modes.hpp"

#include <QFileInfo>
#include <QStringBuilder>

namespace chatterino {

using namespace literals;

Version::Version()
    : version_(CHATTERINO_VERSION)
    , commitHash_(QStringLiteral(CHATTERINO_GIT_HASH))
    , isModified_(CHATTERINO_GIT_MODIFIED == 1)
    , dateOfBuild_(QStringLiteral(CHATTERINO_CMAKE_GEN_DATE))
{
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

    const auto *runtimeVersion = qVersion();
    if (runtimeVersion != QLatin1String{QT_VERSION_STR})
    {
        tags.append(u"Qt "_s QT_VERSION_STR u" (running on " % runtimeVersion %
                    u")");
    }
    else
    {
        tags.append(u"Qt "_s QT_VERSION_STR);
    }

#ifdef _MSC_FULL_VER
    tags.append("MSVC " + QString::number(_MSC_FULL_VER, 10));
#endif
#ifdef CHATTERINO_WITH_CRASHPAD
    tags.append("Crashpad");
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
