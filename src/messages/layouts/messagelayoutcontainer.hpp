#pragma once

#include <memory>
#include <vector>

#include <QPoint>
#include <QRect>

#include "messages/message.hpp"
#include "messages/selection.hpp"

class QPainter;

namespace chatterino {
namespace messages {

namespace layouts {
struct MessageLayoutElement;

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

struct MessageLayoutContainer {
public:
    MessageLayoutContainer();

    Margin margin;
    bool centered;
    bool enableCompactEmotes;

    int getHeight() const;
    int getWidth() const;
    float getScale() const;

    // methods
    void begin(int width, float scale, Message::MessageFlags flags);
    void end();

    void clear();
    void addElement(MessageLayoutElement *element);
    void addElementNoLineBreak(MessageLayoutElement *element);
    void breakLine();
    bool atStartOfLine();
    bool fitsInLine(int width);
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
    float scale;
    int width;
    Message::MessageFlags flags;
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
