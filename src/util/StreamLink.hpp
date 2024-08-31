#pragma once

#include <QString>
#include <QStringList>

#include <stdexcept>
#include <string>

namespace chatterino {

class Exception : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

#ifdef Q_OS_WIN
constexpr inline QStringView STREAMLINK_BINARY_NAME = u"streamlink.exe";
#else
constexpr inline QStringView STREAMLINK_BINARY_NAME = u"streamlink";
#endif

// Open streamlink for given channel, quality and extra arguments
// the "Additional arguments" are fetched and added at the beginning of the
// streamlink call
void openStreamlink(const QString &channelURL, const QString &quality,
                    QStringList extraArguments = QStringList());

// Start opening streamlink for the given channel, reading settings like quality
// from settings and opening a quality dialog if the quality is "Choose"
void openStreamlinkForChannel(const QString &channel);

}  // namespace chatterino
