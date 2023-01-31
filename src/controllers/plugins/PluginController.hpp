#pragma once

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/Singleton.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/commands/CommandController.hpp"
#include "singletons/Paths.hpp"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <semver/semver.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <utility>
#include <vector>

struct lua_State;

namespace chatterino {

struct PluginMeta {
    QString name;
    QString description;
    QString authors;
    QString homepage;

    QString license;
    semver::version version;

    std::vector<QString> tags;

    std::set<QString> libraryPermissions;

    explicit PluginMeta(const QJsonObject &obj)
        : name(obj.value("name").toString("A Plugin with no name"))
        , description(obj.value("description").toString())
        , authors(obj.value("authors").toString())
        , homepage(obj.value("homepage").toString())
        , license(obj.value("license").toString("[unknown]"))

    {
        auto v = semver::from_string_noexcept(
            obj.value("version").toString().toStdString());
        if (v.has_value())
        {
            this->version = v.value();
        }
        else
        {
            this->version = semver::version(0, 0, 0);
            description.append("\nWarning: invalid version");
        }
        for (const auto &t : obj.value("tags").toArray())
        {
            this->tags.push_back(t.toString());
        }
        for (const auto &t : obj.value("library_permissions").toArray())
        {
            this->libraryPermissions.insert(t.toString());
        }
    }
};

class Plugin
{
public:
    QString codename;
    PluginMeta meta;
    bool isDupeName{};

    Plugin(QString codename, lua_State *state, PluginMeta meta,
           const QDir &loadDirectory)
        : codename(std::move(codename))
        , meta(std::move(meta))
        , loadDirectory_(loadDirectory)
        , state_(state)
    {
    }

    bool registerCommand(const QString &name, const QString &functionName)
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

    std::set<QString> listRegisteredCommands()
    {
        std::set<QString> out;
        for (const auto &[name, _] : this->ownedCommands)
        {
            out.insert(name);
        }
        return out;
    }

private:
    QDir loadDirectory_;
    lua_State *state_;

    // maps command name -> function name
    std::map<QString, QString> ownedCommands;

    friend class PluginController;
};

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
