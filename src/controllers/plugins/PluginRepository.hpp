// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/RemotePlugin.hpp"
#    include "util/Expected.hpp"

#    include <pajlada/signals/signal.hpp>
#    include <QString>

#    include <memory>
#    include <span>
#    include <vector>

namespace chatterino {

extern const QString DEFAULT_PLUGIN_REPOSITORY;

class PluginRepository : public std::enable_shared_from_this<PluginRepository>
{
public:
    PluginRepository(const QString &url);

    pajlada::Signals::NoArgSignal onLoaded;

    bool hasErrorOrWarnings() const;

    QString getName() const;
    QUrl getBaseURL() const;
    QString getError() const;
    QString getWarnings() const;

    std::span<const RemotePluginPtr> getPlugins() const;

    void load();

    QUrl fileUrl(const QString &filename) const;

private:
    QString name;
    QUrl baseURL;
    QString error;
    QString warnings;
    std::vector<RemotePluginPtr> plugins;

    ExpectedStr<void> loadFromJson(const QJsonObject &root);
};

}  // namespace chatterino

#endif
