// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/plugins/PluginChannel.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "Application.hpp"
#    include "common/QLogging.hpp"
#    include "controllers/plugins/api/ChannelProviders.hpp"
#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/SelfCallback.hpp"

namespace chatterino {

using namespace Qt::Literals;

class PluginChannelCallbacks
{
public:
    PluginChannelCallbacks() = default;
    PluginChannelCallbacks(const lua::PluginWeakRef &owner,
                           const sol::table &tbl);

    void onSendMessage(const QString &msg) const;
    void onDestroyed() const;

private:
    lua::SelfCallback onSendMessageFn;
    lua::SelfCallback onDestroyedFn;
};

PluginChannelCallbacks::PluginChannelCallbacks(const lua::PluginWeakRef &owner,
                                               const sol::table &tbl)
    : onSendMessageFn(
          owner, tbl.get_or("on_send_message", sol::main_protected_function{}),
          tbl)
    , onDestroyedFn(owner,
                    tbl.get_or("on_destroyed", sol::main_protected_function{}),
                    tbl)
{
}

void PluginChannelCallbacks::onSendMessage(const QString &msg) const
{
    this->onSendMessageFn.tryCall<void>(
        u"PluginChannelCallbacks::onSendMessage", msg);
}

void PluginChannelCallbacks::onDestroyed() const
{
    this->onDestroyedFn.tryCall<void>(u"PluginChannelCallbacks::onDestroyed");
}

PluginChannel::PluginChannel(const QString &name, QString pluginID,
                             QString providerID, QJsonObject arguments)
    : Channel(name, Type::Plugin)
    , arguments_(std::move(arguments))
    , pluginID_(std::move(pluginID))
    , providerID_(std::move(providerID))
    , callbacks_(std::make_unique<PluginChannelCallbacks>())
{
}

PluginChannel::~PluginChannel()
{
    qCDebug(chatterinoLua) << *this << "Destroying";

    this->callbacks_->onDestroyed();
    auto provider = this->provider_.lock();
    if (provider)
    {
        provider->unregisterChannel(*this);
    }
}

std::shared_ptr<PluginChannel> PluginChannel::sharedFromThis()
{
    return std::static_pointer_cast<PluginChannel>(this->shared_from_this());
}

std::weak_ptr<PluginChannel> PluginChannel::weakFromThis()
{
    return this->sharedFromThis();
}

QString PluginChannel::pluginID() const
{
    return this->pluginID_;
}

QString PluginChannel::providerID() const
{
    return this->providerID_;
}

QJsonObject PluginChannel::arguments() const
{
    return this->arguments_;
}

lua::PluginWeakRef PluginChannel::owner() const
{
    return this->owner_;
}

bool PluginChannel::isOwnedBy(const lua::PluginWeakRef &ref) const
{
    return ref.isAlive() && this->owner_ == ref;
}

bool PluginChannel::isOwnedBy(const Plugin &plugin) const
{
    return this->owner_.isAlive() && this->owner_ == plugin.weakRef();
}

ExpectedStr<void> PluginChannel::adopt(
    const std::shared_ptr<lua::api::ChannelProvider> &provider)
{
    if (this->owner_.isAlive())
    {
        return makeUnexpected(u"Owned channels can't be adopted"_s);
    }
    auto ownerPluginRef = provider->owner().strong();
    if (!ownerPluginRef)
    {
        return makeUnexpected(u"Expired plugin"_s);
    }
    if (this->pluginID_ != ownerPluginRef.plugin()->id)
    {
        return makeUnexpected(u"Plugin ID mismatch"_s);
    }
    if (this->providerID_ != provider->id())
    {
        return makeUnexpected(u"Provider ID mismatch"_s);
    }
    auto ret = provider->callbacks().create(this->shared_from_this(),
                                            this->arguments());
    if (!ret)
    {
        return makeUnexpected(u"create() failed"_s);
    }

    qCDebug(chatterinoLua) << *this << "Adopted";

    this->callbacks_ =
        std::make_unique<PluginChannelCallbacks>(provider->owner(), *ret);
    this->owner_ = provider->owner();
    this->provider_ = provider;
    provider->registerChannel(*this);
    return {};
}

void PluginChannel::orphan()
{
    this->callbacks_ = std::make_unique<PluginChannelCallbacks>();
    this->owner_ = {};
    this->provider_ = {};

    qCDebug(chatterinoLua) << *this << "Orphaned";

    auto *app = tryGetApp();
    if (app && !isAppAboutToQuit())
    {
        app->getPlugins()->rememberOrphanedChannel(*this);
    }
}

bool PluginChannel::isEmpty() const
{
    return false;  // We're never empty - even if our name is empty.
}

void PluginChannel::sendMessage(const QString &message)
{
    this->callbacks_->onSendMessage(message);
}

QDebug operator<<(QDebug dbg, const chatterino::PluginChannel &chan)
{
    QDebugStateSaver s(dbg);
    dbg.noquote().nospace()
        << "[PluginChannel " << chan.pluginID_ << "::" << chan.providerID_
        << '#' << chan.getName() << ']';
    return dbg;
}

}  // namespace chatterino

#endif
