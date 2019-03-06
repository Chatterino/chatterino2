#pragma once

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <boost/noncopyable.hpp>

#include <array>
#include <unordered_map>

namespace chatterino
{
    enum class FontStyle : uint8_t {
        Tiny,
        ChatSmall,
        ChatMediumSmall,
        ChatMedium,
        ChatMediumBold,
        ChatMediumItalic,
        ChatLarge,
        ChatVeryLarge,

        UiMedium,
        UiTabs,

        // don't remove this value
        EndType,

        // make sure to update these values accordingly!
        ChatStart = ChatSmall,
        ChatEnd = ChatVeryLarge,
    };

    class Fonts final
    {
    public:
        Fonts();

        // font data gets set in createFontData(...)

        QFont getFont(FontStyle type, float scale);
        QFontMetrics getFontMetrics(FontStyle type, float scale);

    private:
        struct FontData
        {
            FontData(const QFont& _font)
                : font(_font)
                , metrics(_font)
            {
            }

            const QFont font;
            const QFontMetrics metrics;
        };

        struct ChatFontData
        {
            float scale;
            bool italic;
            QFont::Weight weight;
        };

        struct UiFontData
        {
            float size;
            const char* name;
            bool italic;
            QFont::Weight weight;
        };

        FontData& getOrCreateFontData(FontStyle type, float scale);
        FontData createFontData(FontStyle type, float scale);

        std::vector<std::unordered_map<float, FontData>> fontsByType_;
    };

    Fonts* getFonts();
}  // namespace chatterino
