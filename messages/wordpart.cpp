#include "messages/wordpart.h"
#include "messages/word.h"

namespace chatterino {
namespace messages {

WordPart::WordPart(Word &word, int x, int y, int lineNumber, const QString &copyText,
                   bool allowTrailingSpace)
    : _word(word)
    , _copyText(copyText)
    , _text(word.isText() ? _word.getText() : QString())
    , _x(x)
    , _y(y)
    , _width(word.getWidth())
    , _height(word.getHeight())
    , _lineNumber(lineNumber)
    , _trailingSpace(word.hasTrailingSpace() & allowTrailingSpace)
{
}

WordPart::WordPart(Word &word, int x, int y, int width, int height, int lineNumber,
                   const QString &copyText, const QString &customText, bool allowTrailingSpace)
    : _word(word)
    , _copyText(copyText)
    , _text(customText)
    , _x(x)
    , _y(y)
    , _width(width)
    , _height(height)
    , _lineNumber(lineNumber)
    , _trailingSpace(word.hasTrailingSpace() & allowTrailingSpace)
{
}

const Word &WordPart::getWord() const
{
    return _word;
}

int WordPart::getWidth() const
{
    return _width;
}

int WordPart::getHeight() const
{
    return _height;
}

int WordPart::getX() const
{
    return _x;
}

int WordPart::getY() const
{
    return _y;
}

void WordPart::setPosition(int x, int y)
{
    _x = x;
    _y = y;
}

void WordPart::setY(int y)
{
    _y = y;
}

int WordPart::getRight() const
{
    return _x + _width;
}

int WordPart::getBottom() const
{
    return _y + _height;
}

QRect WordPart::getRect() const
{
    return QRect(_x, _y, _width, _height);
}

const QString WordPart::getCopyText() const
{
    return _copyText;
}

int WordPart::hasTrailingSpace() const
{
    return _trailingSpace;
}

const QString &WordPart::getText() const
{
    return _text;
}

int WordPart::getLineNumber()
{
    return _lineNumber;
}
}
}
