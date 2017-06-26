#include "emojis.hpp"
#include "emotemanager.hpp"

#include <QFile>
#include <QStringBuilder>
#include <QTextStream>

namespace chatterino {

QRegularExpression Emojis::findShortCodesRegex(":([-+\\w]+):");

}  // namespace chatterino
