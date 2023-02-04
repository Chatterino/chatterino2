#pragma once
#include "Application.hpp"
#include "controllers/commands/CommandController.hpp"

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <semver/semver.hpp>

#include <set>
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

    explicit PluginMeta(const QJsonObject &obj)
        : name(obj.value("name").toString("A Plugin with no name"))
        , description(obj.value("description").toString("Nothing here"))
        , authors(
              obj.value("authors").toString("[please tell me who made this]"))
        , homepage(obj.value("homepage").toString("[https://example.com]"))
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
            description.append("\nWarning: invalid version. Use semver.");
        }
        for (const auto &t : obj.value("tags").toArray())
        {
            this->tags.push_back(t.toString());
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

    bool registerCommand(const QString &name, const QString &functionName);

    std::set<QString> listRegisteredCommands();

    const QDir &loadDirectory() const
    {
        return this->loadDirectory_;
    }

private:
    QDir loadDirectory_;
    lua_State *state_;

    // maps command name -> function name
    std::map<QString, QString> ownedCommands;

    friend class PluginController;
};
}  // namespace chatterino
