#pragma once

#include <memory>
#include <vector>

#include <QPoint>
#include <QRect>

#include "messages/selection.hpp"

class QPainter;

namespace chatterino {
namespace messages {

namespace layouts {
class MessageLayoutElement;

struct Margin {
    int top;
    int right;
    int bottom;
    int left;

    Margin()
        : Margin(0)
    {
    }

    Margin(int value)
        : Margin(value, value, value, value)
    {
    }

    Margin(int _top, int _right, int _bottom, int _left)
        : top(_top)
        , right(_right)
        , bottom(_bottom)
        , left(_left)
    {
    }
};

class MessageLayoutContainer
{
public:
    MessageLayoutContainer();

    float scale;
    Margin margin;
    bool centered;
    bool enableCompactEmotes;
    int width;

    int getHeight() const;

    // methods
    void clear();
    void addElement(MessageLayoutElement *element);
    void addElementNoLineBreak(MessageLayoutElement *element);
    void breakLine();
    bool atStartOfLine();
    bool fitsInLine(int width);
    void finish();
    MessageLayoutElement *getElementAt(QPoint point);

    // painting
    void paintElements(QPainter &painter);
    void paintAnimatedElements(QPainter &painter, int yOffset);
    void paintSelection(QPainter &painter, int messageIndex, Selection &selection);

    // selection
    int getSelectionIndex(QPoint point);
    int getLastCharacterIndex() const;
    void addSelectionText(QString &str, int from, int to);

private:
    struct Line {
        int startIndex;
        int endIndex;
        int startCharIndex;
        int endCharIndex;
        QRect rect;
    };

    // helpers
    void _addElement(MessageLayoutElement *element);

    // variables
    int line;
    int height;
    int currentX, currentY;
    int charIndex = 0;
    size_t lineStart = 0;
    int lineHeight = 0;
    int spaceWidth = 4;
    std::vector<std::unique_ptr<MessageLayoutElement>> elements;

    std::vector<Line> lines;
};
}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
