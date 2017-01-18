#include "messages/wordpart.h"
#include "messages/word.h"

namespace chatterino {
namespace messages {

WordPart::WordPart(Word &word, int x, int y, const QString &copyText,
                   bool allowTrailingSpace)
    : m_word(word)
    , copyText(copyText)
    , text(word.isText() ? m_word.getText() : QString())
    , x(x)
    , y(y)
    , width(word.getWidth())
    , height(word.getHeight())
    , _trailingSpace(word.hasTrailingSpace() & allowTrailingSpace)
{
}

WordPart::WordPart(Word &word, int x, int y, int width, int height,
                   const QString &copyText, const QString &customText,
                   bool allowTrailingSpace)
    : m_word(word)
    , copyText(copyText)
    , text(customText)
    , x(x)
    , y(y)
    , width(width)
    , height(height)
    , _trailingSpace(word.hasTrailingSpace() & allowTrailingSpace)
{
}
}
}
