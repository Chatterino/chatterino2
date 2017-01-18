#ifndef WORDPART_H
#define WORDPART_H

#include <QRect>
#include <QString>

class Word;

class WordPart
{
public:
    WordPart(Word &getWord, int getX, int getY, const QString &getCopyText,
             bool allowTrailingSpace = true);

    WordPart(Word &getWord, int getX, int getY, int getWidth, int getHeight,
             const QString &getCopyText, const QString &customText,
             bool allowTrailingSpace = true);

    const Word &
    getWord() const
    {
        return m_word;
    }

    int
    getWidth() const
    {
        return width;
    }

    int
    getHeight() const
    {
        return height;
    }

    int
    getX() const
    {
        return x;
    }

    int
    getY() const
    {
        return y;
    }

    void
    setPosition(int x, int y)
    {
        x = x;
        y = y;
    }

    void
    setY(int y)
    {
        y = y;
    }

    int
    getRight() const
    {
        return x + width;
    }

    int
    getBottom() const
    {
        return y + height;
    }

    QRect
    getRect() const
    {
        return QRect(x, y, width, height);
    }

    const QString
    getCopyText() const
    {
        return copyText;
    }

    int
    hasTrailingSpace() const
    {
        return _trailingSpace;
    }

    const QString &
    getText() const
    {
        return text;
    }

private:
    Word &m_word;

    QString copyText;
    QString text;

    int x;
    int y;
    int width;
    int height;

    bool _trailingSpace;
};

#endif  // WORDPART_H
