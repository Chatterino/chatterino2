#include "singletons/fontmanager.hpp"

#include <QDebug>
#include <QtGlobal>

#include "util/assertinguithread.hpp"

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
namespace singletons {

FontManager::FontManager()
    : chatFontFamily("/appearance/currentFontFamily", DEFAULT_FONT_FAMILY)
    , chatFontSize("/appearance/currentFontSize", DEFAULT_FONT_SIZE)
{
    qDebug() << "init FontManager";

    this->chatFontFamily.connect([this](const std::string &newValue, auto) {
        util::assertInGuiThread();

        this->incGeneration();
        for (auto &map : this->fontsByType) {
            map.clear();
        }
        this->fontChanged.invoke();
    });

    this->chatFontSize.connect([this](const int &newValue, auto) {
        util::assertInGuiThread();

        this->incGeneration();
        for (auto &map : this->fontsByType) {
            map.clear();
        }
        this->fontChanged.invoke();
    });

    this->fontsByType.resize((size_t)EndType);
}

QFont FontManager::getFont(FontManager::Type type, float scale)
{
    return this->getOrCreateFontData(type, scale).font;
}

QFontMetrics FontManager::getFontMetrics(FontManager::Type type, float scale)
{
    return this->getOrCreateFontData(type, scale).metrics;
}

int FontManager::getGeneration() const
{
    return this->generation;
}

void FontManager::incGeneration()
{
    this->generation++;
}

FontManager::FontData &FontManager::getOrCreateFontData(Type type, float scale)
{
    util::assertInGuiThread();

    assert(type >= 0 && type < EndType);

    auto &map = this->fontsByType[(size_t)type];

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

FontManager::FontData FontManager::createFontData(Type type, float scale)
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
                              this->chatFontSize.getValue() * data.scale * scale, data.weight,
                              data.italic));
    }

    // normal Ui font (use pt size)
    {
        static std::unordered_map<Type, UiFontData> defaultSize{
            {Tiny, {8, "Monospace", false, QFont::Normal}},
            {UiMedium, {12, DEFAULT_FONT_FAMILY, false, QFont::Normal}},
            {UiTabs, {9, "Segoe UI", false, QFont::Normal}},
        };

        UiFontData &data = defaultSize[type];
        QFont font(data.name, data.size * scale, data.weight, data.italic);
        return FontData(font);
    }
}

}  // namespace singletons
}  // namespace chatterino
