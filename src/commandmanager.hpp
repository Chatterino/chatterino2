#pragma once

#include <QString>

namespace chatterino {
class CommandManager
{
public:
    CommandManager() = delete;

    void execCommand(QString command);
    // void selectSplitRelative(int dx, int dy);
    void moveSplitRelative(int dx, int dy);
};
}
