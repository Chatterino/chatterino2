#pragma once

#include "ab/BaseWidget.hpp"
#include "ui/UiFwd.hpp"

namespace chatterino::ui
{
    class EmotePreview : public ab::BaseWidget
    {
    public:
        EmotePreview();

        void setRoom(Room* room);

    private:
        Room* room{};
    };
}  // namespace chatterino::ui
