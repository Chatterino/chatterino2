#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "word.h"

class AppSettings
{
public:
    static Word::Type
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
    AppSettings();
    static Word::Type wordTypeMask;
};

#endif  // APPSETTINGS_H
