#include "appsettings.h"

Word::Type AppSettings::m_wordTypeMask = Word::Default;

AppSettings::AppSettings()
{
}

bool
AppSettings::isIgnoredEmote(const QString &emote)
{
    return false;
}
