// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/websockets/WebSocketPool.hpp"
#    include "controllers/commands/CommandContext.hpp"
#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/RemotePlugin.hpp"
#    include "util/Expected.hpp"
#    include "util/FunctionRef.hpp"

#    include <pajlada/signals/signal.hpp>
#    include <QDir>
#    include <QFileInfo>
#    include <QJsonArray>
#    include <QJsonObject>
#    include <QString>
#    include <sol/forward.hpp>

#    include <map>
#    include <memory>
#    include <utility>

struct lua_State;

namespace chatterino {

class Settings;
class Paths;

class PluginController
{
    const Paths &paths;

public:
    explicit PluginController(const Paths &paths_);

    void initialize(Settings &settings);

    QString tryExecPluginCommand(const QString &commandName,
                                 const CommandContext &ctx);

    // NOTE: this pointer does not own the Plugin, unique_ptr still owns it
    // This is required to be public because of c functions
    Plugin *getPluginByStatePtr(lua_State *L);

    Plugin *getPluginByID(const QString &id);

    // TODO: make a function that iterates plugins that aren't errored/enabled
    const std::map<QString, std::unique_ptr<Plugin>> &plugins() const;

    /**
     * @brief Reload plugin given by id
     *
     * @param id This is the unique identifier of the plugin, the name of the directory it is in
     */
    bool reload(const QString &id);

    struct RemovePluginArgs {
        bool eraseData = true;
        bool disable = true;
    };

    ExpectedStr<void> removePlugin(const QString &id, RemovePluginArgs args);

    struct DownloadArgs {
        /// The plugin to install.
        RemotePluginPtr remotePlugin;

        /// Optional callback to ask the user about an existing, unrelated plugin.
        ///
        /// If this is not specified, unrelated plugins won't be updated.
        FunctionRef<bool()> onExistingOverwrite;

        /// Required completion callback.
        std::function<void(ExpectedStr<void>)> onDone;

        /// Optional callback to fetch files.
        ///
        /// By default `NetworkRequest` is used.
        FunctionRef<void(QUrl, std::function<void(ExpectedStr<QByteArray>)>)>
            fetchFile;

        /// Update or (re-)install the plugin?
        bool update = false;
    };

    void download(const DownloadArgs &args);

    /**
     * @brief Checks settings to tell if a plugin named by id is enabled.
     *
     * It is the callers responsibility to check Settings::pluginsEnabled
     */
    static bool isPluginEnabled(const QString &id);

    std::pair<bool, QStringList> updateCustomCompletions(
        const QString &query, const QString &fullTextContent,
        int cursorPosition, bool isFirstWord) const;

    WebSocketPool &webSocketPool();

    pajlada::Signals::Signal<Plugin *> onPluginLoaded;
    pajlada::Signals::NoArgSignal onPluginsUpdated;

private:
    void loadPlugins();
    void load(const QFileInfo &index, const QDir &pluginDir,
              const PluginMeta &meta);

    // This function adds lua standard libraries into the state
    void openLibrariesFor(Plugin *plugin);

    void initSol(sol::state_view &lua, Plugin *plugin);

    static void loadChatterinoLib(lua_State *l);
    bool tryLoadFromDir(const QDir &pluginDir);

    void queueChangeNotification();

    ExpectedStr<void> downloadImpl(const DownloadArgs &args);

    ExpectedStr<void> finishDownload(const RemotePlugin &remote,
                                     const std::filesystem::path &pluginDir);

    std::map<QString, std::unique_ptr<Plugin>> plugins_;
    WebSocketPool webSocketPool_;

    std::vector<
        std::pair<std::string, std::function<sol::object(sol::state_view)>>>
        loaders_;

    bool changeNotificationQueued = false;

    std::shared_ptr<bool> lifetime;

    // This is for tests, pay no attention
    friend class PluginControllerAccess;
};

}  // namespace chatterino
#endif
