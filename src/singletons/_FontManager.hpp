#pragma once

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <array>
#include <boost/noncopyable.hpp>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/signal.hpp>
#include <unordered_map>

namespace chatterino {
namespace singletons {

class FontManager : boost::noncopyable
{
public:
    FontManager();

    // font data gets set in createFontData(...)
    enum Type : uint8_t {
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

    QFont getFont(Type type, float scale);
    QFontMetrics getFontMetrics(Type type, float scale);

    pajlada::Settings::Setting<std::string> chatFontFamily;
    pajlada::Settings::Setting<int> chatFontSize;

    pajlada::Signals::NoArgSignal fontChanged;

private:
    struct FontData {
        FontData(const QFont &_font)
            : font(_font)
            , metrics(_font)
        {
        }

        const QFont font;
        const QFontMetrics metrics;
    };

    struct ChatFontData {
        float scale;
        bool italic;
        QFont::Weight weight;
    };

    struct UiFontData {
        float size;
        const char *name;
        bool italic;
        QFont::Weight weight;
    };

    FontData &getOrCreateFontData(Type type, float scale);
    FontData createFontData(Type type, float scale);

    std::vector<std::unordered_map<float, FontData>> fontsByType;
};

}  // namespace singletons

using FontStyle = singletons::FontManager::Type;

}  // namespace chatterino
