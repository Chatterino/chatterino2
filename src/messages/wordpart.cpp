#include "messages/wordpart.hpp"
#include "messages/word.hpp"

namespace chatterino {
namespace messages {

WordPart::WordPart(Word &_word, int _x, int _y, float scale, int _lineNumber,
                   const QString &_copyText, bool _allowTrailingSpace)
    : word(_word)
    , copyText(_copyText)
    , text(_word.isText() ? _word.getText() : QString())
    , x(_x)
    , y(_y)
    , width(_word.getWidth(scale))
    , height(_word.getHeight(scale))
    , lineNumber(_lineNumber)
    , _trailingSpace(!_word.getCopyText().isEmpty() &&
                     _word.hasTrailingSpace() & _allowTrailingSpace)
    , wordCharOffset(0)
{
}

WordPart::WordPart(Word &_word, int _x, int _y, int _width, int _height, int _lineNumber,
                   const QString &_copyText, const QString &_customText, bool _allowTrailingSpace,
                   int _wordCharOffset)
    : word(_word)
    , copyText(_copyText)
    , text(_customText)
    , x(_x)
    , y(_y)
    , width(_width)
    , height(_height)
    , lineNumber(_lineNumber)
    , _trailingSpace(!_word.getCopyText().isEmpty() &&
                     _word.hasTrailingSpace() & _allowTrailingSpace)
    , wordCharOffset(_wordCharOffset)
{
}

const Word &WordPart::getWord() const
{
    return this->word;
}

int WordPart::getWidth() const
{
    return this->width;
}

int WordPart::getHeight() const
{
    return this->height;
}

int WordPart::getX() const
{
    return this->x;
}

int WordPart::getY() const
{
    return this->y;
}

void WordPart::setPosition(int x, int y)
{
    this->x = x;
    this->y = y;
}

void WordPart::setY(int y)
{
    this->y = y;
}

int WordPart::getRight() const
{
    return this->x + this->width;
}

int WordPart::getBottom() const
{
    return this->y + this->height;
}

QRect WordPart::getRect() const
{
    return QRect(this->x, this->y, this->width, this->height - 1);
}

const QString WordPart::getCopyText() const
{
    return this->copyText;
}

int WordPart::hasTrailingSpace() const
{
    return this->_trailingSpace;
}

const QString &WordPart::getText() const
{
    return this->text;
}

int WordPart::getLineNumber() const
{
    return this->lineNumber;
}

int WordPart::getCharacterLength() const
{
    //    return (this->getWord().isImage() ? 1 : this->getText().length()) + (_trailingSpace ? 1 :
    //    0);
    return this->getWord().isImage() ? 2 : this->getText().length() + 1;
}

short WordPart::getCharWidth(int index, float scale) const
{
    return this->getWord().getCharWidth(index + this->wordCharOffset, scale);
}
}  // namespace messages
}  // namespace chatterino
