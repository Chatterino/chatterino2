#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "Application.hpp"
#    include "controllers/commands/CommandController.hpp"

#    include <magic_enum.hpp>
#    include <QDir>
#    include <QJsonArray>
#    include <QJsonObject>
#    include <QString>
#    include <semver/semver.hpp>

#    include <set>
#    include <vector>

struct lua_State;

namespace chatterino {

struct PluginMeta {
    // required fields
    QString name;
    QString description;
    QString authors;
    QString license;
    semver::version version;

    // optional
    QString homepage;
    std::vector<QString> tags;

    bool valid{};
    std::vector<QString> invalidWhy;

    explicit PluginMeta(const QJsonObject &obj)
        : homepage(obj.value("homepage").toString(""))
    {
        auto nameObj = obj.value("name");
        if (!nameObj.isString())
        {
            this->invalidWhy.emplace_back("name is not a string");
            this->valid = false;
        }
        this->name = nameObj.toString();

        auto descrObj = obj.value("description");
        if (!descrObj.isString())
        {
            this->invalidWhy.emplace_back("description is not a string");
            this->valid = false;
        }
        this->description = descrObj.toString();

        auto authorsObj = obj.value("authors");
        if (!authorsObj.isString())
        {
            this->invalidWhy.emplace_back("description is not a string");
            this->valid = false;
        }
        this->authors = authorsObj.toString();

        auto licenseObj = obj.value("license");
        if (!licenseObj.isString())
        {
            this->invalidWhy.emplace_back("license is not a string");
            this->valid = false;
        }
        this->license = licenseObj.toString();

        auto v = semver::from_string_noexcept(
            obj.value("version").toString().toStdString());
        if (v.has_value())
        {
            this->version = v.value();
        }
        else
        {
            this->invalidWhy.emplace_back("unable to parse version");
            this->valid = false;
            this->version = semver::version(0, 0, 0);
        }
        auto tagsObj = obj.value("tags");
        if (!tagsObj.isUndefined())
        {
            if (!tagsObj.isArray())
            {
                this->invalidWhy.emplace_back("tags is not an array");
                this->valid = false;
                return;
            }

            auto tagsArr = tagsObj.toArray();
            for (int i = 0; i < tagsArr.size(); i++)
            {
                const auto &t = tagsArr.at(i);
                if (!t.isString())
                {
                    this->invalidWhy.push_back(
                        QString("tags element #%1 is not a string (it is a %2)")
                            .arg(i)
                            .arg(QString::fromStdString(
                                std::string(magic_enum::enum_name(t.type())))));
                    this->valid = false;
                    return;
                }
                this->tags.push_back(t.toString());
            }
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
#endif
