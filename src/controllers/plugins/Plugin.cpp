#include "Plugin.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "lua.h"

namespace chatterino {
bool Plugin::registerCommand(const QString &name, const QString &functionName)
{
    if (this->ownedCommands.find(name) != this->ownedCommands.end())
    {
        return false;
    }

    auto ok = getApp()->commands->registerPluginCommand(name);
    if (!ok)
    {
        return false;
    }
    this->ownedCommands.insert({name, functionName});
    return true;
}

std::set<QString> Plugin::listRegisteredCommands()
{
    std::set<QString> out;
    for (const auto &[name, _] : this->ownedCommands)
    {
        out.insert(name);
    }
    return out;
}

Plugin::~Plugin()
{
    if (this->state_ != nullptr)
    {
        lua_close(this->state_);
    }
}

}  // namespace chatterino
#endif
