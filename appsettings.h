#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "word.h"

class AppSettings
{
public:
    static Word::Type wordTypeMask() {
        return m_wordTypeMask;
    }

    static bool isIgnoredEmote(const QString& emote);

private:
    AppSettings();
    static Word::Type m_wordTypeMask;
};

#endif // APPSETTINGS_H
