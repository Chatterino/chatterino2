#pragma once

#include "Room.hpp"
#include "ab/BaseWidget.hpp"
#include "ab/util/ConnectionOwner.hpp"
#include "util/QObjectPtr.hpp"

class QLabel;
class QMenu;

namespace chatterino::ui
{
    class SplitHeader : public ab::BaseWidget
    {
        Q_OBJECT

    public:
        SplitHeader();

        void setRoom(Room*);

    private:
        QLabel* label_{};
        Room* room_{};

        QObjectRef<QWidget> menu_;

        ab::ConnectionOwner cowner_;
    };
}  // namespace chatterino::ui
