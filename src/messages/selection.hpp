#pragma once

#include <utility>

namespace chatterino {
namespace messages {

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
        if (this->messageIndex < b.messageIndex) {
            return true;
        }
        if (this->messageIndex == b.messageIndex && this->charIndex < b.charIndex) {
            return true;
        }
        return false;
    }

    bool operator>(const SelectionItem &b) const
    {
        return b.operator<(*this);
    }

    bool operator==(const SelectionItem &b) const
    {
        return this->messageIndex == b.messageIndex && this->charIndex == b.charIndex;
    }
};

struct Selection {
    SelectionItem start;
    SelectionItem end;
    SelectionItem min;
    SelectionItem max;

    Selection() = default;

    Selection(const SelectionItem &start, const SelectionItem &end)
        : start(start)
        , end(end)
        , min(start)
        , max(end)
    {
        if (min > max) {
            std::swap(this->min, this->max);
        }
    }

    bool isEmpty() const
    {
        return this->start == this->end;
    }

    bool isSingleMessage() const
    {
        return this->min.messageIndex == this->max.messageIndex;
    }
};

}  // namespace messages
}  // namespace chatterino
