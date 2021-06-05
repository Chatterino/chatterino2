#pragma once

#include "AttachedPlayer.hpp"
#include "widgets/BaseWidget.hpp"

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

// Open streamlink for given channel, quality and extra arguments
// the "Additional arguments" are fetched and added at the beginning of the
// streamlink call
void openStreamlink(const QString &channelURL, const QString &quality,
                    QStringList extraArguments = QStringList(),
                    bool streamMPV = false);

// Start opening streamlink for the given channel, reading settings like quality
// from settings and opening a quality dialog if the quality is "Choose"
void openStreamlinkForChannel(const QString &channel, bool streamMPV = false);

}  // namespace chatterino
