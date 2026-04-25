// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/SignalCallback.hpp"
#    include "util/Expected.hpp"

#    include <sol/forward.hpp>

#    include <memory>
#    include <span>
#    include <vector>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class PluginChannel;

}  // namespace chatterino

namespace chatterino::lua::api {

namespace channelproviders {

/* @lua-fragment
---@class ChannelProviderArgumentSpecBase
---@field id string
---@field display_name string
---@field tooltip? string

---@class ChannelProviderArgumentSpecText : ChannelProviderArgumentSpecBase
---@field kind "text"
---@field placeholder? string
---@field default? string

---@alias ChannelProviderArgumentSpec ChannelProviderArgumentSpecText

---The plugin local state for a custom/plugin channel.
---This provides both state as well as callbacks for Chatterino to notify the plugin about events.
---Callbacks are queried once when the channel is created. Any changes to the fields are not visible.
---
---When storing state, prefer names with underscores (e.g. `_name`) to avoid collisions in the future.
---@class CustomChannel
---@field on_send_message? fun(self, msg: string) Callback when a message is sent.
---@field on_destroyed? fun(self) Callback when the channel is destroyed (e.g. because the user closed the split).

---@alias ChannelProviderUserArgs table<string, any>

---Arguments passed to `ChannelProviderCallbacks.create`.
---@class ChannelProviderCreateArgs
---@field arguments ChannelProviderUserArgs User provided arguments based on the specification given when registering the provider.

---Callbacks for managing channels of a custom provider.
---@class ChannelProviderCallbacks
---@field get_name fun(arguments: ChannelProviderUserArgs): string Before creating a channel, this is called with the user provided arguments based on the specification given when the provider was registered. This should return the name of the channel to be created. Afterwards, `create` is called with the new channel.
---@field create fun(channel: c2.Channel, args: ChannelProviderCreateArgs): CustomChannel Create a custom channel given the specification from the user. See `CustomChannel` for more info.

---Table to initialize a channel provider.
---
---Chatterino mainly interacts with the provider via the callbacks.
---
---**General Notes**
---
---Channels must have a unique name per provider. Chatterino will deduplicate channels based on the name.
---They are saved with {plugin, provider, name, arguments}.
---`arguments` are the user provided inputs passed as a table to `get_name` and wrapped behind the `arguments` key to `create`.
---Each `id` has a corresponding field.
---
---When the user requests to create a channel the following happens:
---1. `get_name` is called with the user-provided arguments.
---2. If a channel with this combination of {plugin, provider, name} exists. Use that.
---3. A new channel is created. `create` is called with this channel.
---
---The first step is skipped if the channel is loaded from a save, as the channel name is already known.
---
---When a plugin unloads or when Chatterino starts, all created channels are orphaned.
---Once the owning plugin calls `c2.register_channel_provider`, previously orphaned channels are adopted by calling `create`.
---Note that before this, no plugin owns the orphaned channels.
---
---@class ChannelProviderInit
---@field id string A per-plugin unique ID for this provider.
---@field display_name string A name for this provider shown to users.
---@field description? string Description for what this provider does.
---@field arguments ChannelProviderArgumentSpec[] A list of arguments users must provide to create this channel. These are used to create a UI for users.
---@field callbacks ChannelProviderCallbacks

--- Register a channel provider. Channel providers are unique per plugin (through their ID).
---
---@param init ChannelProviderInit Table with arguments to create the provider. See type for more info.
function c2.register_channel_provider(init) end
*/

struct TextSpec {
    QString placeholder;
    QString defaultValue;
};

struct ArgumentSpec {
    explicit ArgumentSpec(const sol::table &tbl);

    QString id;
    QString displayName;
    QString tooltip;

    std::variant<TextSpec> data;
};

void createUserTypes(sol::table &c2);

}  // namespace channelproviders

class ChannelProviderCallbacks
{
public:
    ChannelProviderCallbacks(const PluginWeakRef &owner, const sol::table &tbl);

    std::optional<QString> getName(const QJsonObject &args) const;

    std::optional<sol::table> create(const std::shared_ptr<Channel> &chan,
                                     const QJsonObject &args) const;

private:
    SignalCallback getNameFn;
    SignalCallback createFn;
};

class ChannelProvider
{
public:
    ChannelProvider(const PluginWeakRef &owner, const sol::table &tbl);
    ~ChannelProvider();

    PluginWeakRef owner() const;
    QString id() const;
    QString displayName() const;
    QString description() const;

    std::span<const channelproviders::ArgumentSpec> arguments() const;
    const ChannelProviderCallbacks &callbacks() const;

    void registerChannel(PluginChannel &channel);
    void unregisterChannel(PluginChannel &channel);
    std::shared_ptr<PluginChannel> findExisting(const QString &name) const;

    bool operator==(const ChannelProvider &other) const;

private:
    PluginWeakRef owner_;
    QString id_;
    QString displayName_;
    QString description_;
    std::vector<channelproviders::ArgumentSpec> arguments_;
    ChannelProviderCallbacks callbacks_;

    /// Name => Channel
    std::unordered_map<QString, std::weak_ptr<PluginChannel>> channels;
};

}  // namespace chatterino::lua::api

#endif
