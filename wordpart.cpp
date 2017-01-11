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
{
}

const QString &
WordPart::getText() const
{
    return m_word.getText();
}
