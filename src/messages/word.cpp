#include "messages/word.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/benchmark.hpp"

namespace chatterino {
namespace messages {

// Image word
Word::Word(LazyLoadedImage *image, Flags type, const QString &copytext, const QString &tooltip,
           const Link &link)
    : image(image)
    , _isImage(true)
    , type(type)
    , copyText(copytext)
    , tooltip(tooltip)
    , link(link)
{
}

// Text word
Word::Word(const QString &text, Flags type, const MessageColor &color, FontManager::Type font,
           const QString &copytext, const QString &tooltip, const Link &link)
    : image(nullptr)
    , text(text)
    , color(color)
    , _isImage(false)
    , type(type)
    , copyText(copytext)
    , tooltip(tooltip)
    , font(font)
    , link(link)
{
}

LazyLoadedImage &Word::getImage() const
{
    return *this->image;
}

const QString &Word::getText() const
{
    return this->text;
}

int Word::getWidth(float scale) const
{
    return this->getSize(scale).width();
}

int Word::getHeight(float scale) const
{
    return this->getSize(scale).height();
}

QSize Word::getSize(float scale) const
{
    auto &data = this->getDataByScale(scale);

    if (data.size.isEmpty()) {
        // no size found
        if (this->isText()) {
            QFontMetrics &metrics = this->getFontMetrics(scale);
            data.size.setWidth((int)(metrics.width(this->getText())));
            data.size.setHeight((int)(metrics.height()));
        } else {
            const int mediumTextLineHeight =
                FontManager::getInstance().getFontMetrics(this->font, scale).height();
            const qreal emoteScale = SettingsManager::getInstance().emoteScale.get() * scale;
            const bool scaleEmotesByLineHeight =
                SettingsManager::getInstance().scaleEmotesByLineHeight;

            auto &image = this->getImage();

            qreal w = image.getWidth();
            qreal h = image.getHeight();

            if (scaleEmotesByLineHeight) {
                data.size.setWidth(w * mediumTextLineHeight / h * emoteScale);
                data.size.setHeight(mediumTextLineHeight * emoteScale);
            } else {
                data.size.setWidth(w * image.getScale() * emoteScale);
                data.size.setHeight(h * image.getScale() * emoteScale);
            }
        }
    }

    return data.size;
}

void Word::updateSize()
{
    this->dataByScale.clear();
}

bool Word::isImage() const
{
    return this->_isImage;
}

bool Word::isText() const
{
    return !this->_isImage;
}

const QString &Word::getCopyText() const
{
    return this->copyText;
}

bool Word::hasTrailingSpace() const
{
    return this->_hasTrailingSpace;
}

QFont &Word::getFont(float scale) const
{
    return FontManager::getInstance().getFont(this->font, scale);
}

QFontMetrics &Word::getFontMetrics(float scale) const
{
    return FontManager::getInstance().getFontMetrics(this->font, scale);
}

Word::Flags Word::getFlags() const
{
    return this->type;
}

const QString &Word::getTooltip() const
{
    return this->tooltip;
}

const MessageColor &Word::getTextColor() const
{
    return this->color;
}

const Link &Word::getLink() const
{
    return this->link;
}

int Word::getXOffset() const
{
    return this->xOffset;
}

int Word::getYOffset() const
{
    return this->yOffset;
}

void Word::setOffset(int xOffset, int yOffset)
{
    this->xOffset = std::max(0, xOffset);
    this->yOffset = std::max(0, yOffset);
}

int Word::getCharacterLength() const
{
    return this->isImage() ? 2 : this->getText().length() + 1;
}

short Word::getCharWidth(int index, float scale) const
{
    return this->getCharacterWidthCache(scale).at(index);
}

std::vector<short> &Word::getCharacterWidthCache(float scale) const
{
    auto &data = this->getDataByScale(scale);

    // lock not required because there is only one gui thread
    // std::lock_guard<std::mutex> lock(this->charWidthCacheMutex);

    if (data.charWidthCache.size() == 0 && this->isText()) {
        for (int i = 0; i < this->getText().length(); i++) {
            data.charWidthCache.push_back(
                this->getFontMetrics(scale).charWidth(this->getText(), i));
        }
    }

    // TODO: on font change
    return data.charWidthCache;
}

Word::ScaleDependantData &Word::getDataByScale(float scale) const
{
    // try to find and return data for scale
    for (auto it = this->dataByScale.begin(); it != this->dataByScale.end(); it++) {
        if (it->scale == scale) {
            return *it;
        }
    }

    // create new data element and return that
    this->dataByScale.emplace_back(scale);

    return this->dataByScale.back();
}

}  // namespace messages
}  // namespace chatterino
