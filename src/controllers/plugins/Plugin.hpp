// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/EventType.hpp"
#    include "controllers/plugins/api/HTTPRequest.hpp"
#    include "controllers/plugins/ConnectionManager.hpp"
#    include "controllers/plugins/PluginMeta.hpp"
#    include "controllers/plugins/PluginRef.hpp"

#    include <boost/signals2/signal.hpp>
#    include <QDir>
#    include <QString>
#    include <QUrl>
#    include <semver/semver.hpp>
#    include <sol/forward.hpp>

#    include <memory>
#    include <optional>
#    include <unordered_map>
#    include <unordered_set>
#    include <vector>

struct lua_State;
class QTimer;

namespace chatterino::lua::api {
enum class LogLevel;
}  // namespace chatterino::lua::api

namespace chatterino::lua {
struct SignalCallback;
}  // namespace chatterino::lua

namespace chatterino {

class Plugin
{
public:
    QString id;
    PluginMeta meta;

    Plugin(QString id, lua_State *state, PluginMeta meta,
           const QDir &loadDirectory)
        : id(std::move(id))
        , meta(std::move(meta))
        , loadDirectory_(loadDirectory)
        , state_(state)
        , selfRef_(state ? this : nullptr)
    {
    }

    ~Plugin();

    Plugin(const Plugin &) = delete;
    Plugin(Plugin &&) = delete;
    Plugin &operator=(const Plugin &) = delete;
    Plugin &operator=(Plugin &&) = delete;

    /**
     * @brief Perform all necessary tasks to bind a command name to this plugin
     * @param name name of the command to create
     * @param function the function that should be called when the command is executed
     * @return true if addition succeeded, false otherwise (for example because the command name is already taken)
     */
    bool registerCommand(const QString &name, sol::protected_function function);

    /**
     * @brief Get names of all commands belonging to this plugin
     */
    std::unordered_set<QString> listRegisteredCommands();

    const QDir &loadDirectory() const
    {
        return this->loadDirectory_;
    }

    QDir dataDirectory() const
    {
        return this->loadDirectory_.absoluteFilePath("data");
    }

    std::optional<sol::protected_function> getCompletionCallback()
    {
        if (this->state_ == nullptr || !this->error_.isNull())
        {
            return {};
        }
        auto it =
            this->callbacks.find(lua::api::EventType::CompletionRequested);
        if (it == this->callbacks.end())
        {
            return {};
        }
        return it->second;
    }

    lua::SignalCallback createCallback(sol::main_protected_function pfn);

    /**
     * If the plugin crashes while evaluating the main file, this function will return the error
     */
    QString error()
    {
        return this->error_;
    }

    lua::PluginWeakRef weakRef() const;

    int addTimeout(QTimer *timer);
    void removeTimeout(QTimer *timer);

    bool hasFSPermissionFor(bool write, const QString &path);
    bool hasHTTPPermissionFor(const QUrl &url);
    bool hasNetworkPermission() const;

    void log(lua_State *L, lua::api::LogLevel level, QDebug stream,
             const sol::variadic_args &args);

    sol::state_view state();

    std::map<lua::api::EventType, sol::protected_function> callbacks;

    // In-flight HTTP Requests
    // This is a lifetime hack to ensure they get deleted with the plugin. This relies on the Plugin getting deleted on reload!
    std::vector<std::shared_ptr<lua::api::HTTPRequest>> httpRequests;

    boost::signals2::signal<void()> onUnloaded;
    boost::signals2::signal<void(lua::api::LogLevel, const QString &)> onLog;
    lua::ConnectionManager connections;

private:
    QDir loadDirectory_;
    lua_State *state_;

    lua::PluginRef selfRef_;

    QString error_;

    // maps command name -> function
    std::unordered_map<QString, sol::protected_function> ownedCommands;
    std::vector<QTimer *> activeTimeouts;
    int lastTimerId = 0;

    friend class PluginController;
    friend class PluginControllerAccess;  // this is for tests
};
}  // namespace chatterino
#endif
