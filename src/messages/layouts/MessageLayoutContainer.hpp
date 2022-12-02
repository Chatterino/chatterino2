#pragma once

#include "common/Common.hpp"
#include "common/FlagsEnum.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/Selection.hpp"

#include <QPoint>
#include <QRect>

#include <memory>
#include <vector>

class QPainter;

namespace chatterino {

enum class MessageFlag : int64_t;
enum class FirstWord { Neutral, RTL, LTR };
using MessageFlags = FlagsEnum<MessageFlag>;

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
    MessageLayoutContainer() = default;

    FirstWord first = FirstWord::Neutral;
    bool containsRTL = false;

    int getHeight() const;
    int getWidth() const;
    float getScale() const;

    // methods
    void begin(int width_, float scale_, MessageFlags flags_);
    void end();

    void clear();
    bool canAddElements() const;
    void addElement(MessageLayoutElement *element);
    void addElementNoLineBreak(MessageLayoutElement *element);
    void breakLine();
    bool atStartOfLine();
    bool fitsInLine(int width_);
    // this method is called when a message has an RTL word
    // we need to reorder the words to be shown properly
    // however we don't we to reorder non-text elements like badges, timestamps, username
    // firstTextIndex is the index of the first text element that we need to start the reordering from
    void reorderRTL(int firstTextIndex);
    MessageLayoutElement *getElementAt(QPoint point);

    // painting
    void paintElements(QPainter &painter);
    void paintAnimatedElements(QPainter &painter, int yOffset);
    void paintSelection(QPainter &painter, int messageIndex,
                        Selection &selection, int yOffset);

    // selection
    int getSelectionIndex(QPoint point);
    int getLastCharacterIndex() const;
    int getFirstMessageCharacterIndex() const;
    void addSelectionText(QString &str, uint32_t from, uint32_t to,
                          CopyMode copymode);

    bool isCollapsed();

private:
    struct Line {
        int startIndex;
        int endIndex;
        int startCharIndex;
        int endCharIndex;
        QRect rect;
    };

    // helpers
    /*
    _addElement is called at two stages. first stage is the normal one where we want to add message layout elements to the container.
    If we detect an RTL word in the message, reorderRTL will be called, which is the second stage, where we call _addElement
    again for each layout element, but in the correct order this time, without adding the elemnt to the this->element_ vector.
    Due to compact emote logic, we need the previous element to check if we should change the spacing or not.
    in stage one, this is simply elements_.back(), but in stage 2 that's not the case due to the reordering, and we need to pass the 
    index of the reordered previous element. 
    In stage one we don't need that and we pass -2 to indicate stage one (i.e. adding mode)
    In stage two, we pass -1 for the first element, and the index of the oredered privous element for the rest.
    */
    void _addElement(MessageLayoutElement *element, bool forceAdd = false,
                     int prevIndex = -2);
    bool canCollapse();

    const Margin margin = {4, 8, 4, 8};

    // variables
    float scale_ = 1.f;
    int width_ = 0;
    MessageFlags flags_{};
    int line_ = 0;
    int height_ = 0;
    int currentX_ = 0;
    int currentY_ = 0;
    int charIndex_ = 0;
    size_t lineStart_ = 0;
    int lineHeight_ = 0;
    int spaceWidth_ = 4;
    int textLineHeight_ = 0;
    int dotdotdotWidth_ = 0;
    bool canAddMessages_ = true;
    bool isCollapsed_ = false;
    bool wasPrevReversed_ = false;

    std::vector<std::unique_ptr<MessageLayoutElement>> elements_;
    std::vector<Line> lines_;
};

}  // namespace chatterino
