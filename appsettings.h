#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "word.h"

class AppSettings
{
public:
    static Word::Type
    wordTypeMask()
    {
        return m_wordTypeMask;
    }

    static bool isIgnoredEmote(const QString &emote);
    static qreal
    emoteScale()
    {
        return 1;
    }

    static qreal
    badgeScale()
    {
        return 1;
    }

    static bool
    scaleEmotesByLineHeight()
    {
        return false;
    }

private:
    AppSettings();
    static Word::Type m_wordTypeMask;
};

#endif  // APPSETTINGS_H
