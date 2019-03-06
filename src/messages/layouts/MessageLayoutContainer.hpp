#pragma once

#include <QPoint>
#include <QRect>
#include <memory>
#include <vector>

#include "common/Common.hpp"
#include "common/FlagsEnum.hpp"
#include "messages/Selection.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"

class QPainter;

namespace chatterino
{
    enum class MessageFlag : uint16_t;
    using MessageFlags = FlagsEnum<MessageFlag>;

    struct Margin
    {
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

    struct MessageLayoutContainer
    {
        MessageLayoutContainer() = default;

        Margin margin = {4, 8, 4, 8};
        bool centered = false;
        bool enableCompactEmotes = false;

        int getHeight() const;
        int getWidth() const;
        float getScale() const;

        // methods
        void begin(int width_, float scale_, MessageFlags flags_);
        void end();

        void clear();
        bool canAddElements();
        void addElement(MessageLayoutElement* element);
        void addElementNoLineBreak(MessageLayoutElement* element);
        void breakLine();
        bool atStartOfLine();
        bool fitsInLine(int width_);
        MessageLayoutElement* getElementAt(QPoint point);

        // painting
        void paintElements(QPainter& painter);
        void paintAnimatedElements(QPainter& painter, int yOffset);
        void paintSelection(QPainter& painter, int messageIndex,
            Selection& selection, int yOffset);

        // selection
        int getSelectionIndex(QPoint point);
        int getLastCharacterIndex() const;
        int getFirstMessageCharacterIndex() const;
        void addSelectionText(
            QString& str, int from, int to, CopyMode copymode);

        bool isCollapsed();

    private:
        struct Line
        {
            int startIndex;
            int endIndex;
            int startCharIndex;
            int endCharIndex;
            QRect rect;
        };

        // helpers
        void _addElement(MessageLayoutElement* element, bool forceAdd = false);
        bool canCollapse();

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

        std::vector<std::unique_ptr<MessageLayoutElement>> elements_;
        std::vector<Line> lines_;
    };

}  // namespace chatterino
