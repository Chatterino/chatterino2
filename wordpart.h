#ifndef WORDPART_H
#define WORDPART_H

#include <QRect>
#include <QString>

class Word;

class WordPart
{
public:
    WordPart(Word &word, int x, int y, const QString &copyText,
             bool allowTrailingSpace = true);

    WordPart(Word &word, int x, int y, int width, int height,
             const QString &copyText, const QString &customText,
             bool allowTrailingSpace = true);

    const Word &
    word() const
    {
        return m_word;
    }

    int
    width() const
    {
        return m_width;
    }

    int
    height() const
    {
        return m_height;
    }

    int
    x() const
    {
        return m_x;
    }

    int
    y() const
    {
        return m_y;
    }

    void
    setPosition(int x, int y)
    {
        m_x = x;
        m_y = y;
    }

    void
    setY(int y)
    {
        m_y = y;
    }

    int
    right() const
    {
        return m_x + m_width;
    }

    int
    bottom() const
    {
        return m_y + m_height;
    }

    QRect
    rect() const
    {
        return QRect(m_x, m_y, m_width, m_height);
    }

    const QString
    copyText() const
    {
        return m_copyText;
    }

    int
    hasTrailingSpace() const
    {
        return m_trailingSpace;
    }

    const QString &
    text() const
    {
        return m_text;
    }

private:
    Word &m_word;

    QString m_copyText;
    QString m_text;

    int m_x;
    int m_y;
    int m_width;
    int m_height;

    bool m_trailingSpace;
};

#endif  // WORDPART_H
