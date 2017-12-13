#include "commandmanager.hpp"
#include "windowmanager.hpp"

namespace chatterino {
void CommandManager::execCommand(QString command)
{
//    if (command == "selectr") {
//        selectSplitRelative(false, 1);
//    }
//    if (command == "selectl") {
//        selectSplitRelative(false, -1);
//    }
//    if (command == "selectu") {
//        selectSplitRelative(true, -1);
//    }
//    if (command == "selectd") {
//        selectSplitRelative(true, 1);
//    }

    if (command == "mover") {
        moveSplitRelative(false, 1);
    }
    if (command == "movel") {
        moveSplitRelative(false, -1);
    }
    if (command == "moveu") {
        moveSplitRelative(true, -1);
    }
    if (command == "moved") {
        moveSplitRelative(true, 1);
    }
}

//void CommandManager::selectSplitRelative(bool vertical, int offset)
//{
//    SplitContainer *container = WindowManager::instance->
//
//                                if (vertical)
//}

void CommandManager::moveSplitRelative(int dx, int dy)
{
}
}
