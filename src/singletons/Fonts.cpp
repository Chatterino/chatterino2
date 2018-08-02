#include "singletons/Fonts.hpp"

#include "Application.hpp"
#include "debug/AssertInGuiThread.hpp"
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
    this->fontsByType_.resize(size_t(EndType));
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
}

QFont Fonts::getFont(Fonts::Type type, float scale)
{
    return this->getOrCreateFontData(type, scale).font;
}

QFontMetrics Fonts::getFontMetrics(Fonts::Type type, float scale)
{
    return this->getOrCreateFontData(type, scale).metrics;
}

Fonts::FontData &Fonts::getOrCreateFontData(Type type, float scale)
{
    assertInGuiThread();

    assert(type >= 0 && type < EndType);

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

Fonts::FontData Fonts::createFontData(Type type, float scale)
{
    // check if it's a chat (scale the setting)
    if (type >= ChatStart && type <= ChatEnd) {
        static std::unordered_map<Type, ChatFontData> sizeScale{
            {ChatSmall, {0.6f, false, QFont::Normal}},
            {ChatMediumSmall, {0.8f, false, QFont::Normal}},
            {ChatMedium, {1, false, QFont::Normal}},
            {ChatMediumBold, {1, false, QFont::Medium}},
            {ChatMediumItalic, {1, true, QFont::Normal}},
            {ChatLarge, {1.2f, false, QFont::Normal}},
            {ChatVeryLarge, {1.4f, false, QFont::Normal}},
        };

        auto data = sizeScale[type];
        return FontData(QFont(QString::fromStdString(this->chatFontFamily.getValue()),
                              int(this->chatFontSize.getValue() * data.scale * scale), data.weight,
                              data.italic));
    }

    // normal Ui font (use pt size)
    {
#ifdef Q_OS_MAC
        constexpr float multiplier = 0.8f;
#else
        constexpr float multiplier = 1.f;
#endif

        static std::unordered_map<Type, UiFontData> defaultSize{
            {Tiny, {8, "Monospace", false, QFont::Normal}},
            {UiMedium, {int(9 * multiplier), DEFAULT_FONT_FAMILY, false, QFont::Normal}},
            {UiTabs, {int(9 * multiplier), DEFAULT_FONT_FAMILY, false, QFont::Normal}},
        };

        UiFontData &data = defaultSize[type];
        QFont font(data.name, int(data.size * scale), data.weight, data.italic);
        return FontData(font);
    }
}

}  // namespace chatterino
