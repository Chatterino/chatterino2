#pragma once

#include <tuple>
#include <utility>

namespace chatterino {

struct SelectionItem {
    int messageIndex;
    int charIndex;

    SelectionItem()
    {
        this->messageIndex = 0;
        this->charIndex = 0;
    }

    SelectionItem(int _messageIndex, int _charIndex)
    {
        this->messageIndex = _messageIndex;

        this->charIndex = _charIndex;
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
};

struct DoubleClickSelection {
    int originalStart = 0;
    int originalEnd = 0;
    int origMessageIndex;
    bool selectingLeft = false;
    bool selectingRight = false;
    SelectionItem origStartItem;
    SelectionItem origEndItem;
};

}  // namespace chatterino
