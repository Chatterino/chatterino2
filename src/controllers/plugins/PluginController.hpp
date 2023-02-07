#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/Singleton.hpp"
#    include "controllers/plugins/Plugin.hpp"

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

class Paths;

class PluginController : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;
    void save() override{};
    void callEvery(const QString &functionName);
    void callEveryWithArgs(
        const QString &functionName, int count,
        const std::function<void(const std::unique_ptr<Plugin> &pl,
                                 lua_State *L)> &argCb);

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
    static bool isEnabled(const QString &codename);

private:
    void actuallyInitialize();
    void load(const QFileInfo &index, const QDir &pluginDir,
              const PluginMeta &meta);

    // This function adds lua standard libraries into the state
    static void openLibrariesFor(lua_State *L, const PluginMeta & /*meta*/);
    static void loadChatterinoLib(lua_State *l);
    bool tryLoadFromDir(const QDir &pluginDir);
    std::map<QString, std::unique_ptr<Plugin>> plugins_;
};

};  // namespace chatterino
#endif
