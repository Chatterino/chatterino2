#pragma once

#include <magic_enum.hpp>
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "Application.hpp"
#    include "controllers/plugins/LuaAPI.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "lua.h"

#    include <QDir>
#    include <QString>
#    include <semver/semver.hpp>

#    include <unordered_map>
#    include <unordered_set>
#    include <vector>

struct lua_State;

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

    /**
     * @brief Perform all necessary tasks to bind a command name to this plugin
     * @param name name of the command to create
     * @param functionName name of the function that should be called when the command is executed
     * @return true if addition succeeded, false otherwise (for example because the command name is already taken)
     */
    bool registerCommand(const QString &name, const QString &functionName);

    /**
     * @brief Get names of all commands belonging to this plugin
     */
    std::unordered_set<QString> listRegisteredCommands();

    const QDir &loadDirectory() const
    {
        return this->loadDirectory_;
    }

    boost::optional<
        lua::CallbackFunction<lua::api::CompletionList, QString, QString, bool>>
        getCompletionCallback()
    {
        if (this->state_ == nullptr)
        {
            return boost::none;
        }
        // this uses magic enum to help automatic tooling find usages
        auto typ =
            lua_getfield(this->state_, LUA_REGISTRYINDEX,
                         QString("c2cb-%1")
                             .arg(magic_enum::enum_name<lua::api::EventType>(
                                      lua::api::EventType::CompletionRequested)
                                      .data())
                             .toStdString()
                             .c_str());
        if (typ != LUA_TFUNCTION)
        {
            return boost::none;
        }
        return {lua::CallbackFunction<lua::api::CompletionList, QString,
                                      QString, bool>(lua_gettop(this->state_))};
    }

private:
    QDir loadDirectory_;
    lua_State *state_;

    // maps command name -> function name
    std::unordered_map<QString, QString> ownedCommands;

    friend class PluginController;
};
}  // namespace chatterino
#endif
