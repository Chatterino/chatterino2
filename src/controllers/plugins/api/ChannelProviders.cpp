// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/plugins/api/ChannelProviders.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/api/ChannelRef.hpp"
#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/PluginChannel.hpp"
#    include "controllers/plugins/SolTypes.hpp"

#    include <string>

namespace chatterino::lua::api {

namespace channelproviders {

ArgumentSpec::ArgumentSpec(const sol::table &tbl)
    : id(requiredGet<QString>(tbl, "id"))
    , displayName(requiredGet<QString>(tbl, "display_name"))
    , tooltip(tbl.get_or("tooltip", QString{}))
{
    auto kind = requiredGet<std::string>(tbl, "kind");
    if (kind == "text")
    {
        this->data.emplace<TextSpec>(TextSpec{
            .placeholder = tbl.get_or("placeholder", QString{}),
            .defaultValue = tbl.get_or("default", QString{}),
        });
    }
    else
    {
        throw std::runtime_error("Unknown kind");
    }
}

void createUserTypes(sol::table &c2)
{
    c2.set_function("register_channel_provider", [](ThisPluginState state,
                                                    const sol::table &tbl) {
        auto provider =
            std::make_shared<ChannelProvider>(state.plugin()->weakRef(), tbl);
        state.plugin()->registerChannelProvider(std::move(provider));
    });
}

}  // namespace channelproviders

ChannelProviderCallbacks::ChannelProviderCallbacks(const PluginWeakRef &owner,
                                                   const sol::table &tbl)
    : getNameFn(owner,
                requiredGet<sol::main_protected_function>(tbl, "get_name"))
    , createFn(owner, requiredGet<sol::main_protected_function>(tbl, "create"))
{
}

std::optional<QString> ChannelProviderCallbacks::getName(
    const QJsonObject &args) const
{
    return this->getNameFn.tryCall<QString>(
        u"ChannelProviderCallbacks::getName", args);
}

std::optional<sol::table> ChannelProviderCallbacks::create(
    const std::shared_ptr<Channel> &chan, const QJsonObject &args) const
{
    auto owner = this->createFn.owner().strong();
    if (!owner)
    {
        return std::nullopt;
    }
    auto state = owner.plugin()->state();
    auto argsTable = state.create_table_with("arguments", args);

    return this->createFn.tryCall<sol::table>(
        u"ChannelProviderCallbacks::create", ChannelRef(chan),
        std::move(argsTable));
}

ChannelProvider::ChannelProvider(const PluginWeakRef &owner,
                                 const sol::table &tbl)
    : owner_(owner)
    , id_(requiredGet<QString>(tbl, "id"))
    , displayName_(requiredGet<QString>(tbl, "display_name"))
    , description_(tbl.get_or("description", QString{}))
    , callbacks_(owner, requiredGet<sol::table>(tbl, "callbacks"))
{
    auto arguments = requiredGet<sol::table>(tbl, "arguments");
    for (const auto &[key, value] : arguments)
    {
        auto tableVal = value.as<std::optional<sol::table>>();
        if (!tableVal)
        {
            throw std::runtime_error("Argument is not a table");
        }
        this->arguments_.emplace_back(*tableVal);
    }
}

ChannelProvider::~ChannelProvider()
{
    for (const auto &[name, weak] : this->channels)
    {
        auto strong = weak.lock();
        if (strong)
        {
            strong->orphan();
        }
    }
}

PluginWeakRef ChannelProvider::owner() const
{
    return this->owner_;
}

QString ChannelProvider::id() const
{
    return this->id_;
}

QString ChannelProvider::displayName() const
{
    return this->displayName_;
}

QString ChannelProvider::description() const
{
    return this->description_;
}

std::span<const channelproviders::ArgumentSpec> ChannelProvider::arguments()
    const
{
    return this->arguments_;
}

const ChannelProviderCallbacks &ChannelProvider::callbacks() const
{
    return this->callbacks_;
}

void ChannelProvider::registerChannel(PluginChannel &channel)
{
    if (channel.owner() != this->owner_ ||
        channel.providerID() != channel.providerID())
    {
        assert(false && "Not a channel from this provider");
        return;
    }
    this->channels.emplace(channel.getName(), channel.weakFromThis());
}

void ChannelProvider::unregisterChannel(PluginChannel &channel)
{
    if (channel.owner() != this->owner_ ||
        channel.providerID() != channel.providerID())
    {
        assert(false && "Not a channel from this provider");
        return;
    }
    this->channels.erase(channel.getName());
}

std::shared_ptr<PluginChannel> ChannelProvider::findExisting(
    const QString &name) const
{
    auto it = this->channels.find(name);
    if (it == this->channels.end())
    {
        return nullptr;
    }
    return it->second.lock();
}

bool ChannelProvider::operator==(const ChannelProvider &other) const
{
    return this->owner_ == other.owner_ && this->id_ == other.id_;
}

}  // namespace chatterino::lua::api

#endif
