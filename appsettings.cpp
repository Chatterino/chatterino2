#include "appsettings.h"

Word::Type AppSettings::wordTypeMask = Word::Default;

AppSettings::AppSettings()
{
}

bool
AppSettings::isIgnoredEmote(const QString &emote)
{
    return false;
}
