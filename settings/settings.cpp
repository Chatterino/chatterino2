#include "settings/settings.h"

namespace chatterino {
namespace settings {

messages::Word::Type Settings::wordTypeMask = messages::Word::Default;

Settings::Settings()
{
}

bool
Settings::isIgnoredEmote(const QString &emote)
{
    return false;
}
}
}
