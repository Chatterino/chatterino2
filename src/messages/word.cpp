#include "messages/word.hpp"

namespace chatterino {
namespace messages {

// Image word
Word::Word(LazyLoadedImage *image, Type type, const QString &copytext, const QString &tooltip,
           const Link &link)
    : _image(image)
    , _text()
    , _color()
    , _isImage(true)
    , _type(type)
    , _copyText(copytext)
    , _tooltip(tooltip)
    , _link(link)
    , _characterWidthCache()
{
    image->getWidth();  // professional segfault test
}

// Text word
Word::Word(const QString &text, Type type, const QColor &color, const QString &copytext,
           const QString &tooltip, const Link &link)
    : _image(NULL)
    , _text(text)
    , _color(color)
    , _isImage(false)
    , _type(type)
    , _copyText(copytext)
    , _tooltip(tooltip)
    , _link(link)
    , _characterWidthCache()
{
}

LazyLoadedImage &Word::getImage() const
{
    return *_image;
}

const QString &Word::getText() const
{
    return _text;
}

int Word::getWidth() const
{
    return _width;
}

int Word::getHeight() const
{
    return _height;
}

void Word::setSize(int width, int height)
{
    _width = width;
    _height = height;
}

bool Word::isImage() const
{
    return _isImage;
}

bool Word::isText() const
{
    return !_isImage;
}

const QString &Word::getCopyText() const
{
    return _copyText;
}

bool Word::hasTrailingSpace() const
{
    return _hasTrailingSpace;
}

QFont &Word::getFont() const
{
    return FontManager::getInstance().getFont(_font);
}

QFontMetrics &Word::getFontMetrics() const
{
    return FontManager::getInstance().getFontMetrics(_font);
}

Word::Type Word::getType() const
{
    return _type;
}

const QString &Word::getTooltip() const
{
    return _tooltip;
}

const QColor &Word::getColor() const
{
    return _color;
}

const Link &Word::getLink() const
{
    return _link;
}

int Word::getXOffset() const
{
    return _xOffset;
}

int Word::getYOffset() const
{
    return _yOffset;
}

void Word::setOffset(int xOffset, int yOffset)
{
    _xOffset = std::max(0, xOffset);
    _yOffset = std::max(0, yOffset);
}

std::vector<short> &Word::getCharacterWidthCache() const
{
    return _characterWidthCache;
}

}  // namespace messages
}  // namespace chatterino
