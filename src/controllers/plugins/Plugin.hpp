#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "Application.hpp"
#    include "common/Common.hpp"
#    include "common/network/NetworkCommon.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/PluginPermission.hpp"

#    include <QDir>
#    include <QString>
#    include <QUrl>
#    include <semver/semver.hpp>
#    include <sol/forward.hpp>

#    include <unordered_map>
#    include <unordered_set>
#    include <vector>

struct lua_State;
class QTimer;

namespace chatterino {

struct PluginMeta {
    // for more info on these fields see docs/plugin-info.schema.json

    // display name of the plugin
    QString name;

    // description shown to the user
    QString description;

    // plugin authors shown to the user
    std::vector<QString> authors;

    // license name
    QString license;

    // version of the plugin
    semver::version version;

    // optionally a homepage link
    QString homepage;

    // optionally tags that might help in searching for the plugin
    std::vector<QString> tags;

    std::vector<PluginPermission> permissions;

    // errors that occurred while parsing info.json
    std::vector<QString> errors;

    bool isValid() const
    {
        return this->errors.empty();
    }

    explicit PluginMeta(const QJsonObject &obj);
};

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

    // Note: The CallbackFunction object's destructor will remove the function from the lua stack
    using LuaCompletionCallback =
        lua::CallbackFunction<lua::api::CompletionList,
                              lua::api::CompletionEvent>;
    std::optional<LuaCompletionCallback> getCompletionCallback()
    {
        if (this->state_ == nullptr || !this->error_.isNull())
        {
            return {};
        }
        // this uses magic enum to help automatic tooling find usages
        auto typeName =
            magic_enum::enum_name(lua::api::EventType::CompletionRequested);
        std::string cbName;
        cbName.reserve(5 + typeName.size());
        cbName += "c2cb-";
        cbName += typeName;
        auto typ =
            lua_getfield(this->state_, LUA_REGISTRYINDEX, cbName.c_str());
        if (typ != LUA_TFUNCTION)
        {
            lua_pop(this->state_, 1);
            return {};
        }

        // move
        return std::make_optional<lua::CallbackFunction<
            lua::api::CompletionList, lua::api::CompletionEvent>>(
            this->state_, lua_gettop(this->state_));
    }

    /**
     * If the plugin crashes while evaluating the main file, this function will return the error
     */
    QString error()
    {
        return this->error_;
    }

    int addTimeout(QTimer *timer);
    void removeTimeout(QTimer *timer);

    bool hasFSPermissionFor(bool write, const QString &path);
    bool hasHTTPPermissionFor(const QUrl &url);

private:
    QDir loadDirectory_;
    lua_State *state_;

    QString error_;

    // maps command name -> function
    std::unordered_map<QString, sol::protected_function> ownedCommands;
    std::vector<QTimer *> activeTimeouts;
    int lastTimerId = 0;

    friend class PluginController;
};
}  // namespace chatterino
#endif
