#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/Singleton.hpp"

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <boost/noncopyable.hpp>
#include <pajlada/signals/signal.hpp>

#include <array>
#include <unordered_map>

namespace AB_NAMESPACE {

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
    UiTabs,

    // don't remove this value
    EndType,

    // make sure to update these values accordingly!
    ChatStart = ChatSmall,
    ChatEnd = ChatVeryLarge,
};

class Fonts final : public Singleton
{
public:
    Fonts();

    virtual void initialize(Settings &settings, Paths &paths) override;

    // font data gets set in createFontData(...)

    QFont getFont(FontStyle type, float scale);
    QFontMetrics getFontMetrics(FontStyle type, float scale);

    QStringSetting chatFontFamily;
    IntSetting chatFontSize;

    pajlada::Signals::NoArgSignal fontChanged;
    static Fonts *instance;

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

    FontData &getOrCreateFontData(FontStyle type, float scale);
    FontData createFontData(FontStyle type, float scale);

    std::vector<std::unordered_map<float, FontData>> fontsByType_;
};

Fonts *getFonts();

}  // namespace AB_NAMESPACE
