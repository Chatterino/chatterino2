#pragma once

#include <QRect>
#include <QString>

namespace chatterino {
namespace messages {

class Word;

class WordPart
{
public:
    WordPart(Word &getWord, int getX, int getY, float scale, int _lineNumber,
             const QString &getCopyText, bool allowTrailingSpace = true);

    WordPart(Word &getWord, int getX, int getY, int getWidth, int getHeight, int _lineNumber,
             const QString &getCopyText, const QString &customText, bool allowTrailingSpace = true,
             int wordCharOffset = 0);

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
    int getLineNumber() const;
    int getCharacterLength() const;
    short getCharWidth(int index, float scale) const;

private:
    Word &word;

    QString copyText;
    QString text;

    int x;
    int y;
    int width;
    int height;

    int lineNumber;

    bool _trailingSpace;
    int wordCharOffset;
};

}  // namespace messages
}  // namespace chatterino
