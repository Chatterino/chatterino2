#include "fontmanager.hpp"

#include <QDebug>

namespace chatterino {

FontManager::FontManager()
    : currentFontFamily("/appearance/currentFontFamily", "Arial")
    , currentFontSize("/appearance/currentFontSize", 12)
//    , currentFont(this->currentFontFamily.getValue().c_str(), currentFontSize.getValue())
{
    this->currentFontFamily.connect([this](const std::string &newValue, auto) {
        this->incGeneration();
        //        this->currentFont.setFamily(newValue.c_str());
        this->currentFontByDpi.clear();
        this->fontChanged.invoke();
    });
    this->currentFontSize.connect([this](const int &newValue, auto) {
        this->incGeneration();
        //        this->currentFont.setSize(newValue);
        this->currentFontByDpi.clear();
        this->fontChanged.invoke();
    });
}

QFont &FontManager::getFont(Type type, float dpi)
{
    //    return this->currentFont.getFont(type);
    return this->getCurrentFont(dpi).getFont(type);
}

QFontMetrics &FontManager::getFontMetrics(Type type, float dpi)
{
    //    return this->currentFont.getFontMetrics(type);
    return this->getCurrentFont(dpi).getFontMetrics(type);
}

FontManager::FontData &FontManager::Font::getFontData(Type type)
{
    switch (type) {
        case Small:
            return this->small;
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

FontManager::Font &FontManager::getCurrentFont(float dpi)
{
    for (auto it = this->currentFontByDpi.begin(); it != this->currentFontByDpi.end(); it++) {
        if (it->first == dpi) {
            return it->second;
        }
    }
    this->currentFontByDpi.push_back(std::make_pair(
        dpi,
        Font(this->currentFontFamily.getValue().c_str(), this->currentFontSize.getValue() * dpi)));

    return this->currentFontByDpi.back().second;
}
}  // namespace chatterino
