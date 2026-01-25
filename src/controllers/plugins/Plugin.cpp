// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/Plugin.hpp"

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/PluginPermission.hpp"

#    include <lua.h>
#    include <QLoggingCategory>
#    include <QUrl>
#    include <sol/sol.hpp>

#    include <algorithm>
#    include <unordered_map>
#    include <unordered_set>

namespace chatterino {

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
    this->onUnloaded();

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

void Plugin::log(lua_State *L, lua::api::LogLevel level, QDebug stream,
                 const sol::variadic_args &args)
{
    stream.noquote();
    stream << "[" + this->id + ":" + this->meta.name + "]";
    QString fullMessage;
    for (const auto &arg : args)
    {
        auto s = lua::toString(L, arg.stack_index());
        stream << s;

        if (!fullMessage.isEmpty())
        {
            fullMessage.append(' ');
        }
        fullMessage.append(s);

        // Remove this from our stack
        lua_pop(L, 1);
    }

    this->onLog(level, fullMessage);
}

sol::state_view Plugin::state()
{
    return {this->state_};
}

bool Plugin::hasNetworkPermission() const
{
    return std::ranges::any_of(this->meta.permissions, [](const auto &p) {
        return p.type == PluginPermission::Type::Network;
    });
}

}  // namespace chatterino
#endif
