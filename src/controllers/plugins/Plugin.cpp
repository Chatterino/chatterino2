#include "Plugin.hpp"
#ifdef CHATTERINO_HAVE_PLUGINS

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
}  // namespace chatterino
#endif
