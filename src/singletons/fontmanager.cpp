#include "singletons/fontmanager.hpp"

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
namespace singletons {

FontManager::FontManager()
    : currentFontFamily("/appearance/currentFontFamily", DEFAULT_FONT_FAMILY)
    , currentFontSize("/appearance/currentFontSize", DEFAULT_FONT_SIZE)
//    , currentFont(this->currentFontFamily.getValue().c_str(), currentFontSize.getValue())
{
    qDebug() << "init FontManager";

    this->currentFontFamily.connect([this](const std::string &newValue, auto) {
        this->incGeneration();
        //        this->currentFont.setFamily(newValue.c_str());
        this->currentFontByScale.clear();
        this->fontChanged.invoke();
    });
    this->currentFontSize.connect([this](const int &newValue, auto) {
        this->incGeneration();
        //        this->currentFont.setSize(newValue);
        this->currentFontByScale.clear();
        this->fontChanged.invoke();
    });
}

FontManager &FontManager::getInstance()
{
    static FontManager instance;
    return instance;
}

QFont &FontManager::getFont(FontManager::Type type, float scale)
{
    //    return this->currentFont.getFont(type);
    return this->getCurrentFont(scale).getFont(type);
}

QFontMetrics &FontManager::getFontMetrics(FontManager::Type type, float scale)
{
    //    return this->currentFont.getFontMetrics(type);
    return this->getCurrentFont(scale).getFontMetrics(type);
}

FontManager::FontData &FontManager::Font::getFontData(FontManager::Type type)
{
    switch (type) {
        case Tiny:
            return this->tiny;
        case Small:
            return this->small;
        case MediumSmall:
            return this->mediumSmall;
        case Medium:
            return this->medium;
        case MediumBold:
            return this->mediumBold;
        case MediumItalic:
            return this->mediumItalic;
        case Large:
            return this->large;
        case VeryLarge:
            return this->veryLarge;
        default:
            qDebug() << "Unknown font type:" << type << ", defaulting to medium";
            return this->medium;
    }
}

QFont &FontManager::Font::getFont(Type type)
{
    return this->getFontData(type).font;
}

QFontMetrics &FontManager::Font::getFontMetrics(Type type)
{
    return this->getFontData(type).metrics;
}

FontManager::Font &FontManager::getCurrentFont(float scale)
{
    for (auto it = this->currentFontByScale.begin(); it != this->currentFontByScale.end(); it++) {
        if (it->first == scale) {
            return it->second;
        }
    }
    this->currentFontByScale.push_back(
        std::make_pair(scale, Font(this->currentFontFamily.getValue().c_str(),
                                   this->currentFontSize.getValue() * scale)));

    return this->currentFontByScale.back().second;
}

}  // namespace singletons
}  // namespace chatterino
