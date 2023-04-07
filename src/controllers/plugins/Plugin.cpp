#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/Plugin.hpp"

#    include "controllers/commands/CommandController.hpp"

#    include <lua.h>
#    include <magic_enum.hpp>
#    include <QJsonArray>
#    include <QJsonObject>

#    include <unordered_map>
#    include <unordered_set>

namespace chatterino {

PluginMeta::PluginMeta(const QJsonObject &obj)
{
    auto homepageObj = obj.value("homepage");
    if (homepageObj.isString())
    {
        this->homepage = homepageObj.toString();
    }
    else if (!homepageObj.isUndefined())
    {
        QString type = magic_enum::enum_name(homepageObj.type()).data();
        this->errors.emplace_back(
            QString("homepage is defined but is not a string (its type is %1)")
                .arg(type));
    }
    auto nameObj = obj.value("name");
    if (nameObj.isString())
    {
        this->name = nameObj.toString();
    }
    else
    {
        QString type = magic_enum::enum_name(nameObj.type()).data();
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
        QString type = magic_enum::enum_name(descrObj.type()).data();
        this->errors.emplace_back(
            QString("description is not a string (its type is %1)").arg(type));
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
                QString type = magic_enum::enum_name(t.type()).data();
                this->errors.push_back(
                    QString("authors element #%1 is not a string (it is a %2)")
                        .arg(i)
                        .arg(type));
                break;
            }
            this->authors.push_back(t.toString());
        }
    }
    else
    {
        QString type = magic_enum::enum_name(authorsObj.type()).data();
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
        QString type = magic_enum::enum_name(licenseObj.type()).data();
        this->errors.emplace_back(
            QString("license is not a string (its type is %1)").arg(type));
    }

    auto verObj = obj.value("version");
    if (verObj.isString())
    {
        auto v = semver::from_string_noexcept(verObj.toString().toStdString());
        if (v.has_value())
        {
            this->version = v.value();
        }
        else
        {
            this->errors.emplace_back("unable to parse version (use semver)");
            this->version = semver::version(0, 0, 0);
        }
    }
    else
    {
        QString type = magic_enum::enum_name(verObj.type()).data();
        this->errors.emplace_back(
            QString("version is not a string (its type is %1)").arg(type));
        this->version = semver::version(0, 0, 0);
    }
    auto tagsObj = obj.value("tags");
    if (!tagsObj.isUndefined())
    {
        if (!tagsObj.isArray())
        {
            QString type = magic_enum::enum_name(tagsObj.type()).data();
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
                QString type = magic_enum::enum_name(t.type()).data();
                this->errors.push_back(
                    QString("tags element #%1 is not a string (its type is %2)")
                        .arg(i)
                        .arg(type));
                return;
            }
            this->tags.push_back(t.toString());
        }
    }
}

bool Plugin::registerCommand(const QString &name, const QString &functionName)
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

std::unordered_set<QString> Plugin::listRegisteredCommands()
{
    std::unordered_set<QString> out;
    for (const auto &[name, _] : this->ownedCommands)
    {
        out.insert(name);
    }
    return out;
}

Plugin::~Plugin()
{
    if (this->state_ != nullptr)
    {
        lua_close(this->state_);
    }
}

}  // namespace chatterino
#endif
