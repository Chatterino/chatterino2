#include "commandmanager.hpp"
#include "windowmanager.hpp"

namespace chatterino {
void CommandManager::execCommand(QString command)
{
    if (command == "selectr") {
        selectSplitRelative(1, 0);
    }
    if (command == "selectl") {
        selectSplitRelative(-1, 0);
    }
    if (command == "selectu") {
        selectSplitRelative(0, -1);
    }
    if (command == "selectd") {
        selectSplitRelative(0, 1);
    }

    if (command == "mover") {
        moveSplitRelative(1, 0);
    }
    if (command == "movel") {
        moveSplitRelative(-1, 0);
    }
    if (command == "moveu") {
        moveSplitRelative(0, -1);
    }
    if (command == "moved") {
        moveSplitRelative(0, 1);
    }
}

void CommandManager::selectSplitRelative(int dx, int dy)
{
    WindowManager::instance->getCurrentWindow().getNotebook().getSelectedPage();
}

void CommandManager::moveSplitRelative(int dx, int dy)
{
}
}
