#include "messages/word.hpp"

namespace chatterino {
namespace messages {

// Image word
Word::Word(const QString &imageURL, Type type, const QString &tooltip, const Link &link)
    : imageURL(imageURL)
    , _isImage(true)
    , type(type)
    , tooltip(tooltip)
    , link(link)
{
}

// Text word
Word::Word(const QString &text, Type type, const MessageColor &color, const QString &tooltip,
           const Link &link)
    : text(text)
    , color(color)
    , _isImage(false)
    , type(type)
    , tooltip(tooltip)
    , link(link)
{
}

const QString &Word::getImageURL() const
{
    return this->imageURL;
}

const QString &Word::getText() const
{
    return this->text;
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

}  // namespace messages
}  // namespace chatterino
