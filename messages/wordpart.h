#ifndef WORDPART_H
#define WORDPART_H

#include <QRect>
#include <QString>

namespace  chatterino {
namespace  messages {

class Word;

class WordPart
{
public:
    WordPart(Word &getWord, int getX, int getY, int _lineNumber, const QString &getCopyText,
             bool allowTrailingSpace = true);

    WordPart(Word &getWord, int getX, int getY, int getWidth, int getHeight, int _lineNumber,
             const QString &getCopyText, const QString &customText, bool allowTrailingSpace = true);

    const Word &getWord() const;
    int getWidth() const;
    int getHeight() const;
    int getX() const;
    int getY() const;
    void setPosition(int _x, int _y);
    void setY(int _y);
    int getRight() const;
    int getBottom() const;
    QRect getRect() const;
    const QString getCopyText() const;
    int hasTrailingSpace() const;
    const QString &getText() const;
    int getLineNumber();

private:
    Word &_word;

    QString _copyText;
    QString _text;

    int _x;
    int _y;
    int _width;
    int _height;

    int _lineNumber;

    bool _trailingSpace;
};
}
}

#endif  // WORDPART_H
