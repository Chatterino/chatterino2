#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/Singleton.hpp"
#    include "controllers/commands/CommandContext.hpp"
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

    QString tryExecPluginCommand(const QString &commandName,
                                 const CommandContext &ctx);

    // NOTE: this pointer does not own the Plugin, unique_ptr still owns it
    // This is required to be public because of c functions
    Plugin *getPluginByStatePtr(lua_State *L);

    // TODO: make a function that iterates plugins that aren't errored/enabled
    const std::map<QString, std::unique_ptr<Plugin>> &plugins() const;

    /**
     * @brief Reload plugin given by id
     *
     * @param id This is the unique identifier of the plugin, the name of the directory it is in
     */
    bool reload(const QString &id);

    /**
     * @brief Checks settings to tell if a plugin named by id is enabled.
     *
     * It is the callers responsibility to check Settings::pluginsEnabled
     */
    static bool isPluginEnabled(const QString &id);

    std::pair<bool, QStringList> updateCustomCompletions(
        const QString &query, const QString &fullTextContent,
        int cursorPosition, bool isFirstWord) const;

private:
    void loadPlugins();
    void load(const QFileInfo &index, const QDir &pluginDir,
              const PluginMeta &meta);

    // This function adds lua standard libraries into the state
    static void openLibrariesFor(lua_State *L, const PluginMeta & /*meta*/,
                                 const QDir &pluginDir);
    static void loadChatterinoLib(lua_State *l);
    bool tryLoadFromDir(const QDir &pluginDir);
    std::map<QString, std::unique_ptr<Plugin>> plugins_;
};

}  // namespace chatterino
#endif
