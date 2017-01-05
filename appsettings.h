#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "word.h"

class AppSettings
{
public:
    Word::Type wordTypeMask() {
        return m_wordTypeMask;
    }

private:
    AppSettings();
    Word::Type m_wordTypeMask = Word::Default;
};

#endif // APPSETTINGS_H
