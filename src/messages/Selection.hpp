#pragma once

#include <tuple>
#include <utility>

namespace chatterino
{
    struct SelectionItem
    {
        int messageIndex;
        int charIndex;

        SelectionItem()
            : messageIndex(0)
            , charIndex(0)
        {
        }

        SelectionItem(int messageIndex, int charIndex)
            : messageIndex(messageIndex)
            , charIndex(charIndex)
        {
        }

        bool operator<(const SelectionItem& b) const
        {
            return std::tie(this->messageIndex, this->charIndex) <
                   std::tie(b.messageIndex, b.charIndex);
        }

        bool operator>(const SelectionItem& b) const
        {
            return !this->operator==(b) && b.operator<(*this);
        }

        bool operator==(const SelectionItem& b) const
        {
            return this->messageIndex == b.messageIndex &&
                   this->charIndex == b.charIndex;
        }

        bool operator!=(const SelectionItem& b) const
        {
            return !this->operator==(b);
        }
    };

    struct Selection
    {
        SelectionItem start;
        SelectionItem end;

        Selection() = default;

        Selection(const SelectionItem& pos1, const SelectionItem& pos2)
            : start(pos1)
            , end(pos2)
        {
            if (pos1 > pos2)
                std::swap(this->start, this->end);
        }

        bool isEmpty() const
        {
            return this->start == this->end;
        }

        bool isSingleMessage() const
        {
            return this->start.messageIndex == this->end.messageIndex;
        }
    };
}  // namespace chatterino
