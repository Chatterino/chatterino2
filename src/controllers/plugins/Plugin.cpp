#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/Plugin.hpp"

#    include "common/network/NetworkCommon.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/PluginPermission.hpp"
#    include "util/QMagicEnum.hpp"

#    include <lua.h>
#    include <magic_enum/magic_enum.hpp>
#    include <QJsonArray>
#    include <QJsonObject>
#    include <QLoggingCategory>
#    include <QUrl>
#    include <sol/sol.hpp>

#    include <algorithm>
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
        auto type = qmagicenum::enumName(homepageObj.type());
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
        auto type = qmagicenum::enumName(nameObj.type());
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
        auto type = qmagicenum::enumName(descrObj.type());
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
                auto type = qmagicenum::enumName(t.type());
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
        auto type = qmagicenum::enumName(authorsObj.type());
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
        auto type = qmagicenum::enumName(licenseObj.type());
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
        auto type = qmagicenum::enumName(verObj.type());
        this->errors.emplace_back(
            QString("version is not a string (its type is %1)").arg(type));
        this->version = semver::version(0, 0, 0);
    }
    auto permsObj = obj.value("permissions");
    if (!permsObj.isUndefined())
    {
        if (!permsObj.isArray())
        {
            auto type = qmagicenum::enumName(permsObj.type());
            this->errors.emplace_back(
                QString("permissions is not an array (its type is %1)")
                    .arg(type));
            return;
        }

        auto permsArr = permsObj.toArray();
        for (int i = 0; i < permsArr.size(); i++)
        {
            const auto &t = permsArr.at(i);
            if (!t.isObject())
            {
                auto type = qmagicenum::enumName(t.type());
                this->errors.push_back(QString("permissions element #%1 is not "
                                               "an object (its type is %2)")
                                           .arg(i)
                                           .arg(type));
                return;
            }
            auto parsed = PluginPermission(t.toObject());
            if (parsed.isValid())
            {
                // ensure no invalid permissions slip through this
                this->permissions.push_back(parsed);
            }
            else
            {
                for (const auto &err : parsed.errors)
                {
                    this->errors.push_back(
                        QString("permissions element #%1: %2").arg(i).arg(err));
                }
            }
        }
    }

    auto tagsObj = obj.value("tags");
    if (!tagsObj.isUndefined())
    {
        if (!tagsObj.isArray())
        {
            auto type = qmagicenum::enumName(tagsObj.type());
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
                auto type = qmagicenum::enumName(t.type());
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

bool Plugin::registerCommand(const QString &name,
                             sol::protected_function function)
{
    if (this->ownedCommands.find(name) != this->ownedCommands.end())
    {
        return false;
    }

    auto ok = getApp()->getCommands()->registerPluginCommand(name);
    if (!ok)
    {
        return false;
    }
    this->ownedCommands.emplace(name, std::move(function));
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
    for (auto *timer : this->activeTimeouts)
    {
        QObject::disconnect(timer, nullptr, nullptr, nullptr);
        timer->deleteLater();
    }
    this->httpRequests.clear();
    qCDebug(chatterinoLua) << "Destroyed" << this->activeTimeouts.size()
                           << "timers for plugin" << this->id
                           << "while destroying the object";
    this->activeTimeouts.clear();
    if (this->state_ != nullptr)
    {
        // clearing this after the state is gone is not safe to do
        this->ownedCommands.clear();
        this->callbacks.clear();
        lua_close(this->state_);
    }
    assert(this->ownedCommands.empty() &&
           "This must be empty or destructor of sol::protected_function would "
           "explode malloc structures later");
    assert(this->callbacks.empty() &&
           "This must be empty or destructor of sol::protected_function would "
           "explode malloc structures later");
}
int Plugin::addTimeout(QTimer *timer)
{
    this->activeTimeouts.push_back(timer);
    return ++this->lastTimerId;
}

void Plugin::removeTimeout(QTimer *timer)
{
    for (auto it = this->activeTimeouts.begin();
         it != this->activeTimeouts.end(); ++it)
    {
        if (*it == timer)
        {
            this->activeTimeouts.erase(it);
            break;
        }
    }
}

bool Plugin::hasFSPermissionFor(bool write, const QString &path)
{
    auto canon = QUrl(this->dataDirectory().absolutePath() + "/");
    if (!canon.isParentOf(path))
    {
        return false;
    }

    using PType = PluginPermission::Type;
    auto typ = write ? PType::FilesystemWrite : PType::FilesystemRead;

    return std::ranges::any_of(this->meta.permissions, [=](const auto &p) {
        return p.type == typ;
    });
}

bool Plugin::hasHTTPPermissionFor(const QUrl &url)
{
    auto proto = url.scheme();
    if (proto != "http" && proto != "https")
    {
        qCWarning(chatterinoLua).nospace()
            << "Plugin " << this->id << " (" << this->meta.name
            << ") is trying to use a non-http protocol";
        return false;
    }

    return std::ranges::any_of(this->meta.permissions, [](const auto &p) {
        return p.type == PluginPermission::Type::Network;
    });
}

}  // namespace chatterino
#endif
