#ifndef FONTS_H
#define FONTS_H

#include "QFont"

class Fonts
{
public:
    enum Type : char {
        Medium,
        MediumBold,
        MediumItalic,
        Small,
        Large,
        VeryLarge
    };

    static QFont& getFont(Type type);

private:
    Fonts();

    static QFont* medium;
    static QFont* mediumBold;
    static QFont* mediumItalic;
    static QFont* small;
    static QFont* large;
    static QFont* veryLarge;
};

#endif // FONTS_H
