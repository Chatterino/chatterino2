#include "util/XDGDesktopFile.hpp"

#include "util/XDGDirectory.hpp"

#include <QDir>
#include <QFile>

#include <functional>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace {

chatterino::XDGDesktopFile::Group const EMPTY_GROUP;

}  // namespace

namespace chatterino {

XDGDesktopFile::XDGDesktopFile(char const *filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        return;
    }
    this->_exists = true;

    std::optional<std::reference_wrapper<Group>> currentGroup;

    while (!file.atEnd())
    {
        auto lineBytes = file.readLine().trimmed();
        // comments are not guaranteed to be valid UTF-8, so check before
        // decoding the line
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
            currentGroup = this->_groups[groupName];
        }
        else
        {
            if (!currentGroup.has_value())
            {
                // no group header yet, any other data is against spec and
                // should be ignored
                continue;
            }

            auto equals = line.indexOf('=');
            if (equals == -1)
            {
                // line is not a group header or a key value pair, ignore
                continue;
            }

            auto key = QStringView(line).left(equals).trimmed().toString();
            // QStringView.mid() does not do bounds checking before qt 5.15, so
            // we have to do it ourselves
            auto valueStart = equals + 1;
            auto value =
                valueStart < line.size()
                    ? QStringView(line).mid(valueStart).trimmed().toString()
                    : QString("");

            // existing keys are against spec, so we can overwrite them with
            // wild abandon
            currentGroup.value().get()[key] = value;
        }
    }
}

XDGDesktopFile::XDGDesktopFile(QString const &filename)
    : XDGDesktopFile(filename.isNull() ? ""
                                       : filename.toLocal8Bit().constData())
{
}

XDGDesktopFile::Group const &XDGDesktopFile::operator[](
    QString const &key) const
{
    auto group = this->_groups.find(key);
    if (group != this->_groups.end())
    {
        return group->second;
    }
    return EMPTY_GROUP;
}

std::optional<XDGDesktopFile> XDGDesktopFile::findDesktopId(
    const QString &desktopId)
{
    for (const auto &dataDir : getXDGDirectories(XDGDirectoryType::Data))
    {
        auto fileName =
            QDir::cleanPath(dataDir + QDir::separator() + "applications" +
                            QDir::separator() + desktopId);
        XDGDesktopFile desktopFile(fileName);
        if (desktopFile.exists())
        {
            return desktopFile;
        }
    }
    return {};
}

}  // namespace chatterino

#endif
