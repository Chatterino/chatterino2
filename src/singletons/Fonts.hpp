#pragma once

#include "pajlada/settings/settinglistener.hpp"

#include <pajlada/signals/signal.hpp>
#include <QFont>
#include <QFontMetrics>

#include <unordered_map>
#include <vector>

namespace chatterino {

class Settings;
class Paths;

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
    UiMediumBold,
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
    explicit Fonts(Settings &settings);

    // font data gets set in createFontData(...)

    QFont getFont(FontStyle type, float scale);
    QFontMetrics getFontMetrics(FontStyle type, float scale);

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
    };

    struct UiFontData {
        float size;
        const char *name;
        bool italic;
        int weight;
    };

    FontData &getOrCreateFontData(FontStyle type, float scale);
    static FontData createFontData(FontStyle type, float scale);

    std::vector<std::unordered_map<float, FontData>> fontsByType_;

    pajlada::SettingListener fontChangedListener;
};

}  // namespace chatterino
