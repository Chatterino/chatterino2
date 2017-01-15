#include "wordpart.h"
#include "word.h"

WordPart::WordPart(Word &word, int x, int y, const QString &copyText,
                   bool allowTrailingSpace)
    : m_word(word)
    , m_copyText(copyText)
    , m_x(x)
    , m_y(y)
    , m_width(word.width())
    , m_height(word.height())
    , m_trailingSpace(word.hasTrailingSpace() & allowTrailingSpace)
    , m_text(word.isText() ? m_word.getText() : QString())
{
}

WordPart::WordPart(Word &word, int x, int y, int width, int height,
                   const QString &copyText, const QString &customText,
                   bool allowTrailingSpace)
    : m_word(word)
    , m_copyText(copyText)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_trailingSpace(word.hasTrailingSpace() & allowTrailingSpace)
    , m_text(customText)
{
}
