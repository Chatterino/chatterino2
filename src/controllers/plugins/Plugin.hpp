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

    explicit PluginMeta(const QJsonObject &obj)
    {
        auto homepageObj = obj.value("homepage");
        if (homepageObj.isString())
        {
            this->homepage = homepageObj.toString();
        }
        else if (!homepageObj.isUndefined())
        {
            auto type = QString::fromStdString(
                std::string(magic_enum::enum_name(homepageObj.type())));
            this->errors.emplace_back(
                QString(
                    "homepage is defined but is not a string (its type is %1)")
                    .arg(type));
        }
        auto nameObj = obj.value("name");
        if (nameObj.isString())
        {
            this->name = nameObj.toString();
        }
        else
        {
            auto type = QString::fromStdString(
                std::string(magic_enum::enum_name(nameObj.type())));
            this->errors.emplace_back(
                QString("name is not a string (its type is %1)").arg(type));
        }

        auto descrObj = obj.value("description");
        if (descrObj.isString())
        {
            this->description = descrObj.toString();
        }
        else
        {
            auto type = QString::fromStdString(
                std::string(magic_enum::enum_name(descrObj.type())));
            this->errors.emplace_back(
                QString("description is not a string (its type is %1)")
                    .arg(type));
        }

        auto authorsObj = obj.value("authors");
        if (authorsObj.isArray())
        {
            auto authorsArr = authorsObj.toArray();
            for (int i = 0; i < authorsArr.size(); i++)
            {
                const auto &t = authorsArr.at(i);
                if (!t.isString())
                {
                    this->errors.push_back(
                        QString(
                            "authors element #%1 is not a string (it is a %2)")
                            .arg(i)
                            .arg(QString::fromStdString(
                                std::string(magic_enum::enum_name(t.type())))));
                    break;
                }
                this->authors.push_back(t.toString());
            }
        }
        else
        {
            auto type = QString::fromStdString(
                std::string(magic_enum::enum_name(authorsObj.type())));
            this->errors.emplace_back(
                QString("authors is not an array (its type is %1)").arg(type));
        }

        auto licenseObj = obj.value("license");
        if (licenseObj.isString())
        {
            this->license = licenseObj.toString();
        }
        else
        {
            auto type = QString::fromStdString(
                std::string(magic_enum::enum_name(licenseObj.type())));
            this->errors.emplace_back(
                QString("license is not a string (its type is %1)").arg(type));
        }

        auto verObj = obj.value("version");
        if (verObj.isString())
        {
            auto v =
                semver::from_string_noexcept(verObj.toString().toStdString());
            if (v.has_value())
            {
                this->version = v.value();
            }
            else
            {
                this->errors.emplace_back(
                    "unable to parse version (use semver)");
                this->version = semver::version(0, 0, 0);
            }
        }
        else
        {
            auto type = QString::fromStdString(
                std::string(magic_enum::enum_name(verObj.type())));
            this->errors.emplace_back(
                QString("version is not a string (its type is %1)").arg(type));
            this->version = semver::version(0, 0, 0);
        }
        auto tagsObj = obj.value("tags");
        if (!tagsObj.isUndefined())
        {
            if (!tagsObj.isArray())
            {
                auto type = QString::fromStdString(
                    std::string(magic_enum::enum_name(licenseObj.type())));
                this->errors.emplace_back(
                    QString("tags is not an array (its type is %1)").arg(type));
                return;
            }

            auto tagsArr = tagsObj.toArray();
            for (int i = 0; i < tagsArr.size(); i++)
            {
                const auto &t = tagsArr.at(i);
                if (!t.isString())
                {
                    this->errors.push_back(
                        QString(
                            "tags element #%1 is not a string (its type is %2)")
                            .arg(i)
                            .arg(QString::fromStdString(
                                std::string(magic_enum::enum_name(t.type())))));
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
