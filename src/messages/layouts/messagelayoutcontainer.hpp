#pragma once

#include <memory>
#include <vector>

#include <QPoint>
#include <QRect>

class QPainter;

namespace chatterino {
namespace messages {
class Selection;

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

private:
    // helpers
    void _addElement(MessageLayoutElement *element);

    // variables
    int line;
    int height;
    int currentX, currentY;
    size_t lineStart = 0;
    int lineHeight = 0;
    int spaceWidth = 4;
    std::vector<std::unique_ptr<MessageLayoutElement>> elements;

    struct Line {
        int startIndex;
        QRect rect;
    };

    std::vector<Line> lines;
};
}  // namespace layouts
}  // namespace messages
}  // namespace chatterino
