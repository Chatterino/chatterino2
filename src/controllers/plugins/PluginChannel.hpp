// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "common/Channel.hpp"
#    include "controllers/plugins/PluginRef.hpp"
#    include "util/Expected.hpp"

#    include <QJsonObject>

#    include <memory>


namespace chatterino {

namespace lua::api {

class ChannelProvider;

}  // namespace lua::api

class PluginChannelCallbacks;
class PluginChannel : public Channel
{
public:
    PluginChannel(const QString &name, QString pluginID, QString providerID,
                  QJsonObject arguments);
    ~PluginChannel() override;

    /// \defgroup SharedPtr utilities
    /// \{
    std::shared_ptr<PluginChannel> sharedFromThis();
    std::weak_ptr<PluginChannel> weakFromThis();
    /// \}

    /// \defgroup Descriptor accessors
    /// \{
    QString pluginID() const;
    QString providerID() const;
    QJsonObject arguments() const;
    /// \}

    /// \defgroup Plugin association
    /// \{
    lua::PluginWeakRef owner() const;
    bool isOwnedBy(const lua::PluginWeakRef &ref) const;
    bool isOwnedBy(const Plugin &plugin) const;

    std::weak_ptr<lua::api::ChannelProvider> provider() const;

    /// Try to adopt this channel by a channel provider.
    [[nodiscard]] ExpectedStr<void> adopt(
        const std::shared_ptr<lua::api::ChannelProvider> &provider);
    void orphan();
    /// \}

    /// \defgroup Inherited methods
    /// \{
    bool isEmpty() const override;
    void sendMessage(const QString &message) override;
    /// \}

private:
    QJsonObject arguments_;
    QString pluginID_;
    QString providerID_;

    lua::PluginWeakRef owner_;
    std::weak_ptr<lua::api::ChannelProvider> provider_;
    std::unique_ptr<PluginChannelCallbacks> callbacks_;

    friend QDebug operator<<(QDebug dbg, const PluginChannel &chan);
};

}  // namespace chatterino

#endif
