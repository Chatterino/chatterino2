#include "singletons/Fonts.hpp"

#include "Application.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"

#include <QDebug>
#include <QtGlobal>

#ifdef Q_OS_WIN32
#define DEFAULT_FONT_FAMILY "Segoe UI"
#define DEFAULT_FONT_SIZE 10
#else
#ifdef Q_OS_MACOS
#define DEFAULT_FONT_FAMILY "Helvetica Neue"
#define DEFAULT_FONT_SIZE 12
#else
#define DEFAULT_FONT_FAMILY "Arial"
#define DEFAULT_FONT_SIZE 11
#endif
#endif

namespace chatterino {

Fonts::Fonts()
    : chatFontFamily("/appearance/currentFontFamily", DEFAULT_FONT_FAMILY)
    , chatFontSize("/appearance/currentFontSize", DEFAULT_FONT_SIZE)
{
    this->fontsByType_.resize(size_t(FontStyle::EndType));
}

void Fonts::initialize(Settings &, Paths &)
{
    this->chatFontFamily.connect([this](const std::string &, auto) {
        assertInGuiThread();

        for (auto &map : this->fontsByType_) {
            map.clear();
        }
        this->fontChanged.invoke();
    });

    this->chatFontSize.connect([this](const int &, auto) {
        assertInGuiThread();

        for (auto &map : this->fontsByType_) {
            map.clear();
        }
        this->fontChanged.invoke();
    });

    getSettings()->boldScale.connect([this](const int &, auto) {
        assertInGuiThread();

        getApp()->windows->incGeneration();

        for (auto &map : this->fontsByType_) {
            map.clear();
        }
        this->fontChanged.invoke();
    });
}

QFont Fonts::getFont(FontStyle type, float scale)
{
    return this->getOrCreateFontData(type, scale).font;
}

QFontMetrics Fonts::getFontMetrics(FontStyle type, float scale)
{
    return this->getOrCreateFontData(type, scale).metrics;
}

Fonts::FontData &Fonts::getOrCreateFontData(FontStyle type, float scale)
{
    assertInGuiThread();

    assert(type < FontStyle::EndType);

    auto &map = this->fontsByType_[size_t(type)];

    // find element
    auto it = map.find(scale);
    if (it != map.end()) {
        // return if found

        return it->second;
    }

    // emplace new element
    auto result = map.emplace(scale, this->createFontData(type, scale));
    assert(result.second);

    return result.first->second;
}

Fonts::FontData Fonts::createFontData(FontStyle type, float scale)
{
    // check if it's a chat (scale the setting)
    if (type >= FontStyle::ChatStart && type <= FontStyle::ChatEnd) {
        static std::unordered_map<FontStyle, ChatFontData> sizeScale{
            {FontStyle::ChatSmall, {0.6f, false, QFont::Normal}},
            {FontStyle::ChatMediumSmall, {0.8f, false, QFont::Normal}},
            {FontStyle::ChatMedium, {1, false, QFont::Normal}},
            {FontStyle::ChatMediumBold,
             {1, false,
              QFont::Weight(getSettings()->boldScale.getValue())}},
            {FontStyle::ChatMediumItalic, {1, true, QFont::Normal}},
            {FontStyle::ChatLarge, {1.2f, false, QFont::Normal}},
            {FontStyle::ChatVeryLarge, {1.4f, false, QFont::Normal}},
        };
        sizeScale[FontStyle::ChatMediumBold] = {
            1, false, QFont::Weight(getSettings()->boldScale.getValue())};
        auto data = sizeScale[type];
        return FontData(
            QFont(QString::fromStdString(this->chatFontFamily.getValue()),
                  int(this->chatFontSize.getValue() * data.scale * scale),
                  data.weight, data.italic));
    }

    // normal Ui font (use pt size)
    {
#ifdef Q_OS_MAC
        constexpr float multiplier = 0.8f;
#else
        constexpr float multiplier = 1.f;
#endif

        static std::unordered_map<FontStyle, UiFontData> defaultSize{
            {FontStyle::Tiny, {8, "Monospace", false, QFont::Normal}},
            {FontStyle::UiMedium,
             {int(9 * multiplier), DEFAULT_FONT_FAMILY, false, QFont::Normal}},
            {FontStyle::UiTabs,
             {int(9 * multiplier), DEFAULT_FONT_FAMILY, false, QFont::Normal}},
        };

        UiFontData &data = defaultSize[type];
        QFont font(data.name, int(data.size * scale), data.weight, data.italic);
        return FontData(font);
    }
}

}  // namespace chatterino
