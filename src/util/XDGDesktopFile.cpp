#include "util/XDGDesktopFile.hpp"

#include "util/XDGDirectory.hpp"

#include <QDir>
#include <QFile>

#include <functional>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace chatterino {

XDGDesktopFile::XDGDesktopFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        return;
    }
    this->valid = true;

    std::optional<std::reference_wrapper<XDGEntries>> entries;

    while (!file.atEnd())
    {
        auto lineBytes = file.readLine().trimmed();

        // Ignore comments & empty lines
        if (lineBytes.startsWith('#') || lineBytes.size() == 0)
        {
            continue;
        }

        auto line = QString::fromUtf8(lineBytes);

        if (line.startsWith('['))
        {
            // group header
            auto end = line.indexOf(']', 1);
            if (end == -1 || end == 1)
            {
                // malformed header - either empty or no closing bracket
                continue;
            }
            auto groupName = line.mid(1, end - 1);

            // it is against spec for the group name to already exist, but the
            // parsing behavior for that case is not specified. operator[] will
            // result in duplicate groups being merged, which makes the most
            // sense for a read-only parser
            entries = this->groups[groupName];

            continue;
        }

        // group entry
        if (!entries.has_value())
        {
            // no group header yet, entry before a group header is against spec
            // and should be ignored
            continue;
        }

        auto delimiter = line.indexOf('=');
        if (delimiter == -1)
        {
            // line is not a group header or a key value pair, ignore it
            continue;
        }

        auto key = QStringView(line).left(delimiter).trimmed().toString();
        // QStringView.mid() does not do bounds checking before qt 5.15, so
        // we have to do it ourselves
        auto valueStart = delimiter + 1;
        QString value;
        if (valueStart < line.size())
        {
            value = QStringView(line).mid(valueStart).trimmed().toString();
        }

        // existing keys are against spec, so we can overwrite them with
        // wild abandon
        entries->get().emplace(key, value);
    }
}

XDGEntries XDGDesktopFile::getEntries(const QString &groupHeader) const
{
    auto group = this->groups.find(groupHeader);
    if (group != this->groups.end())
    {
        return group->second;
    }

    return {};
}

std::optional<XDGDesktopFile> XDGDesktopFile::findDesktopFile(
    const QString &desktopFileID)
{
    for (const auto &dataDir : getXDGDirectories(XDGDirectoryType::Data))
    {
        auto fileName =
            QDir::cleanPath(dataDir + QDir::separator() + "applications" +
                            QDir::separator() + desktopFileID);
        XDGDesktopFile desktopFile(fileName);
        if (desktopFile.isValid())
        {
            return desktopFile;
        }
    }
    return {};
}

}  // namespace chatterino

#endif
