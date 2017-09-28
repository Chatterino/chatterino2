#include "messages/word.hpp"

namespace chatterino {
namespace messages {

// Image word
Word::Word(LazyLoadedImage *image, Type type, const QString &copytext, const QString &tooltip,
           const Link &link)
    : image(image)
    , _isImage(true)
    , type(type)
    , copyText(copytext)
    , tooltip(tooltip)
    , link(link)
{
    image->getWidth();  // professional segfault test
}

// Text word
Word::Word(const QString &text, Type type, const MessageColor &color, const QString &copytext,
           const QString &tooltip, const Link &link)
    : image(nullptr)
    , text(text)
    , color(color)
    , _isImage(false)
    , type(type)
    , copyText(copytext)
    , tooltip(tooltip)
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

int Word::getWidth() const
{
    return this->width;
}

int Word::getHeight() const
{
    return this->height;
}

void Word::setSize(int width, int height)
{
    this->width = width;
    this->height = height;
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

QFont &Word::getFont() const
{
    return FontManager::getInstance().getFont(this->font);
}

QFontMetrics &Word::getFontMetrics() const
{
    return FontManager::getInstance().getFontMetrics(this->font);
}

Word::Type Word::getType() const
{
    return this->type;
}

const QString &Word::getTooltip() const
{
    return this->tooltip;
}

const MessageColor &Word::getColor() const
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

const QString &Word::getEmoteURL() const
{
    return emoteURL;
}

std::vector<short> &Word::getCharacterWidthCache() const
{
    // lock not required because there is only one gui thread
    // std::lock_guard<std::mutex> lock(this->charWidthCacheMutex);

    if (this->charWidthCache.size() == 0 && this->isText()) {
        for (int i = 0; i < this->getText().length(); i++) {
            this->charWidthCache.push_back(this->getFontMetrics().charWidth(this->getText(), i));
        }
    }

    // TODO: on font change
    return this->charWidthCache;
}

}  // namespace messages
}  // namespace chatterino
