#pragma once

#include <memory>

#include "Room.hpp"
#include "ab/BaseWindow.hpp"

namespace ab
{
    class Notebook;
}

namespace chatterino
{
    class Application;

    class SelectChannel : public QObject
    {
        Q_OBJECT

    public:
        void showDialog(Application&, const Room& currentRoom);
        void closeDialog();

    signals:
        void accepted(const RoomPtr& room);

    private:
        ab::Notebook* notebook_{};

        std::unique_ptr<ab::Dialog> dialog_;
        std::vector<QWidget*> pages_;
    };
}  // namespace chatterino
