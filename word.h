#ifndef WORD_H
#define WORD_H

#include "lazyloadedimage.h"
#include "fonts.h"

#include <QRect>
#include <QString>

class Word
{
public:
    enum Type : long int {
        None = 0,
        Misc = 1,
        Text = 2,

        TimestampNoSeconds   = 4,
        TimestampWithSeconds = 8,

        TwitchEmoteImage = 0x10,
        TwitchEmoteText = 0x20,
        BttvEmoteImage = 0x40,
        BttvEmoteText = 0x80,
        BttvGifEmoteImage = 0x100,
        BttvGifEmoteText = 0x200,
        FfzEmoteImage = 0x400,
        FfzEmoteText = 0x800,

        Bits = 0x1000,
        BitsAnimated = 0x2000,

        BadgeStaff = 0x4000,
        BadgeAdmin = 0x8000,
        BadgeGlobalMod = 0x10000,
        BadgeModerator = 0x20000,
        BadgeTurbo = 0x40000,
        BadgeBroadcaster = 0x80000,
        BadgePremium = 0x100000,
        BadgeChatterino = 0x200000,
        BadgeBits = 0x400000,
    };

    explicit Word(LazyLoadedImage* m_image, Type type, const QString& copytext, const QString& tooltip = "");
    explicit Word(const QString& m_text, Type type, const QString& copytext, const QString& tooltip = "");

    ~Word();

    LazyLoadedImage& getImage() {
        return *m_image;
    }

    const QString& getText() {
        return m_text;
    }

    int width() {
        return m_width;
    }

    int height() {
        return m_height;
    }

    int x() {
        return m_x;
    }

    int y() {
        return m_y;
    }

    QRect rect() {
        return QRect(m_x, m_y, m_width, m_height);
    }

    bool isImage() {
        return m_isImage;
    }

    const QString& copyText() {
        return m_copyText;
    }

    bool hasTrailingSpace() {
        return m_hasTrailingSpace;
    }

    QFont& getFont() {
        return Fonts::getFont(m_font);
    }

    Type type() {
        return m_type;
    }

    const QString& tooltip() {
        return m_tooltip;
    }

private:
    LazyLoadedImage* m_image;
    QString m_text;
    bool m_isImage;

    Type m_type;
    QString m_copyText;
    QString m_tooltip;
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    bool m_hasTrailingSpace;
    Fonts::Type m_font = Fonts::Medium;
};

#endif // WORD_H
