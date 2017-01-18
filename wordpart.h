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
        return this->m_word;
    }

    int
    getWidth() const
    {
        return this->width;
    }

    int
    getHeight() const
    {
        return this->height;
    }

    int
    getX() const
    {
        return this->x;
    }

    int
    getY() const
    {
        return this->y;
    }

    void
    setPosition(int x, int y)
    {
        this->x = x;
        this->y = y;
    }

    void
    setY(int y)
    {
        this->y = y;
    }

    int
    getRight() const
    {
        return this->x + this->width;
    }

    int
    getBottom() const
    {
        return this->y + this->height;
    }

    QRect
    getRect() const
    {
        return QRect(this->x, this->y, this->width, this->height);
    }

    const QString
    getCopyText() const
    {
        return this->copyText;
    }

    int
    hasTrailingSpace() const
    {
        return this->_trailingSpace;
    }

    const QString &
    getText() const
    {
        return this->text;
    }

private:
    Word& m_word;

    QString copyText;
    QString text;

    int x;
    int y;
    int width;
    int height;

    bool _trailingSpace;
};

#endif  // WORDPART_H
