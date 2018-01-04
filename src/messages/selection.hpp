#pragma once

namespace chatterino {
namespace messages {
struct SelectionItem {
    int messageIndex;
    int charIndex;

    SelectionItem()
    {
        messageIndex = charIndex = 0;
    }

    SelectionItem(int _messageIndex, int _charIndex)
    {
        this->messageIndex = _messageIndex;
        this->charIndex = _charIndex;
    }

    bool isSmallerThan(const SelectionItem &other) const
    {
        return this->messageIndex < other.messageIndex ||
               (this->messageIndex == other.messageIndex && this->charIndex < other.charIndex);
    }

    bool equals(const SelectionItem &other) const
    {
        return this->messageIndex == other.messageIndex && this->charIndex == other.charIndex;
    }
};

struct Selection {
    SelectionItem start;
    SelectionItem end;
    SelectionItem min;
    SelectionItem max;

    Selection()
    {
    }

    Selection(const SelectionItem &start, const SelectionItem &end)
        : start(start)
        , end(end)
        , min(start)
        , max(end)
    {
        if (max.isSmallerThan(min)) {
            std::swap(this->min, this->max);
        }
    }

    bool isEmpty() const
    {
        return this->start.equals(this->end);
    }

    bool isSingleMessage() const
    {
        return this->min.messageIndex == this->max.messageIndex;
    }
};
}
}
