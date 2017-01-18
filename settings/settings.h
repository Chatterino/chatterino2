#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "messages/word.h"

namespace chatterino {
namespace settings {

class Settings
{
public:
    static messages::Word::Type
    getWordTypeMask()
    {
        return wordTypeMask;
    }

    static bool isIgnoredEmote(const QString &emote);

    static qreal
    getEmoteScale()
    {
        return 1;
    }

    static qreal
    getBadgeScale()
    {
        return 1;
    }

    static bool
    getScaleEmotesByLineHeight()
    {
        return false;
    }

private:
    Settings();
    static messages::Word::Type wordTypeMask;
};
}
}

#endif  // APPSETTINGS_H
