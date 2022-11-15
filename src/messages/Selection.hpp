#pragma once

#include <cstdint>
#include <tuple>
#include <utility>

namespace chatterino {

struct SelectionItem {
    uint32_t messageIndex{0};
    uint32_t charIndex{0};

    SelectionItem() = default;

    SelectionItem(uint32_t _messageIndex, uint32_t _charIndex)
        : messageIndex(_messageIndex)
        , charIndex(_charIndex)
    {
    }

    bool operator<(const SelectionItem &b) const
    {
        return std::tie(this->messageIndex, this->charIndex) <
               std::tie(b.messageIndex, b.charIndex);
    }

    bool operator>(const SelectionItem &b) const
    {
        return !this->operator==(b) && b.operator<(*this);
    }

    bool operator==(const SelectionItem &b) const
    {
        return this->messageIndex == b.messageIndex &&
               this->charIndex == b.charIndex;
    }

    bool operator!=(const SelectionItem &b) const
    {
        return this->operator==(b);
    }
};

struct Selection {
    SelectionItem start;
    SelectionItem end;
    SelectionItem selectionMin;
    SelectionItem selectionMax;

    Selection() = default;

    Selection(const SelectionItem &start, const SelectionItem &end)
        : start(start)
        , end(end)
        , selectionMin(start)
        , selectionMax(end)
    {
        if (selectionMin > selectionMax)
        {
            std::swap(this->selectionMin, this->selectionMax);
        }
    }

    bool isEmpty() const
    {
        return this->start == this->end;
    }

    bool isSingleMessage() const
    {
        return this->selectionMin.messageIndex ==
               this->selectionMax.messageIndex;
    }

    // Shift all message selection indices `offset` back
    void shiftMessageIndex(uint32_t offset)
    {
        if (offset > this->selectionMin.messageIndex)
        {
            this->selectionMin.messageIndex = 0;
        }
        else
        {
            this->selectionMin.messageIndex -= offset;
        }

        if (offset > this->selectionMax.messageIndex)
        {
            this->selectionMax.messageIndex = 0;
        }
        else
        {
            this->selectionMax.messageIndex -= offset;
        }

        if (offset > this->start.messageIndex)
        {
            this->start.messageIndex = 0;
        }
        else
        {
            this->start.messageIndex -= offset;
        }

        if (offset > this->end.messageIndex)
        {
            this->end.messageIndex = 0;
        }
        else
        {
            this->end.messageIndex -= offset;
        }
    }
};

struct DoubleClickSelection {
    uint32_t originalStart{0};
    uint32_t originalEnd{0};
    uint32_t origMessageIndex{0};
    bool selectingLeft{false};
    bool selectingRight{false};
    SelectionItem origStartItem;
    SelectionItem origEndItem;
};

}  // namespace chatterino
