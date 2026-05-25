// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/PluginMeta.hpp"

#    include <QMetaType>

namespace chatterino {

class PluginRepository;

struct RemotePlugin {
    explicit RemotePlugin(const QJsonObject &root);

    QString id;
    QString downloadPath;
    QUrl downloadURL;
    PluginMeta meta;
    std::weak_ptr<PluginRepository> repository;
};

using RemotePluginPtr = std::shared_ptr<const RemotePlugin>;

}  // namespace chatterino

// Allow use in QVariant
Q_DECLARE_METATYPE(chatterino::RemotePluginPtr)

#endif
