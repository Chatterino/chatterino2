#pragma once

#include "Room.hpp"
#include "ab/BaseWidget.hpp"

class QLabel;

namespace chatterino::ui
{
    class ResizingTextEdit;

    class SplitInput : public ab::BaseWidget
    {
        Q_OBJECT

    public:
        SplitInput();

        ResizingTextEdit* edit() const;
        void setRoom(Room*);

    private:
        void updateInputLength();

        Room* room_{};
        ResizingTextEdit* edit_{};
        QLabel* inputLength_{};
    };
}  // namespace chatterino::ui
