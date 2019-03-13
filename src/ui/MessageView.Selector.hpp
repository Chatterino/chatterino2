#pragma once

#include "messages/Selection.hpp"

namespace chatterino::ui
{
    class Selector : public QObject
    {
    public:
        [[nodiscard]] Selection selection();

        void start(const SelectionItem& pos, const SelectionItem& wordStart,
            const SelectionItem& wordEnd);
        void moveRegular(const SelectionItem& pos);
        void moveWord(const SelectionItem& start, const SelectionItem& end);
        void clear();

    private:
        SelectionItem startRegular_;
        SelectionItem startWordLeft_;
        SelectionItem startWordRight_;

        Selection selection_;
    };
}  // namespace chatterino::ui
