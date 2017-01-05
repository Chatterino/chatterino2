#include "fonts.h"

#define DEFAULT_FONT "Arial"

QFont* Fonts::medium       = new QFont(DEFAULT_FONT);
QFont* Fonts::mediumBold   = new QFont(DEFAULT_FONT);
QFont* Fonts::mediumItalic = new QFont(DEFAULT_FONT);
QFont* Fonts::small        = new QFont(DEFAULT_FONT);
QFont* Fonts::large        = new QFont(DEFAULT_FONT);
QFont* Fonts::veryLarge    = new QFont(DEFAULT_FONT);

Fonts::Fonts()
{

}

QFont& Fonts::getFont(Type type)
{
    if (type == Medium      ) return *medium      ;
    if (type == MediumBold  ) return *mediumBold  ;
    if (type == MediumItalic) return *mediumItalic;
    if (type == Small       ) return *small       ;
    if (type == Large       ) return *large       ;
    if (type == VeryLarge   ) return *veryLarge   ;

    return *medium;
}
