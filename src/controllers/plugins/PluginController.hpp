#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/Singleton.hpp"
#    include "controllers/plugins/Plugin.hpp"
#    include "singletons/Paths.hpp"

#    include <QDir>
#    include <QFileInfo>
#    include <QJsonArray>
#    include <QJsonObject>
#    include <QString>

#    include <algorithm>
#    include <map>
#    include <memory>
#    include <utility>
#    include <vector>

struct lua_State;

namespace chatterino {

class PluginController : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;
    void save() override{};
    void callEvery(const QString &functionName);
    void callEveryWithArgs(
        const QString &functionName, int count,
        std::function<void(const std::unique_ptr<Plugin> &pl, lua_State *L)>
            argCb);

    QString tryExecPluginCommand(const QString &commandName,
                                 const CommandContext &ctx);

    // NOTE: this pointer does not own the Plugin, unique_ptr still owns it
    // This is required to be public because of c functions
    Plugin *getPluginByStatePtr(lua_State *L)
    {
        for (auto &[name, plugin] : this->plugins_)
        {
            if (plugin->state_ == L)
            {
                return plugin.get();
            }
        }
        return nullptr;
    }

    const std::map<QString, std::unique_ptr<Plugin>> &plugins() const
    {
        return this->plugins_;
    }

    bool reload(const QString &codename);
    bool isEnabled(const QString &codename);

private:
    void actuallyInitialize();
    void load(QFileInfo index, QDir pluginDir, PluginMeta meta);
    void loadChatterinoLib(lua_State *l);

    // This function adds lua standard libraries into the state
    void openLibrariesFor(lua_State *L, PluginMeta meta);
    bool tryLoadFromDir(const QDir &pluginDir);
    std::map<QString, std::unique_ptr<Plugin>> plugins_;
};

};  // namespace chatterino
#endif
