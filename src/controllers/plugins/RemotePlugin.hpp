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

    /// ID of this plugin.
    ///
    /// Key: "id"
    QString id;

    /// Path for this plugin relative to the repository's base URL.
    ///
    /// This may be empty. In that case, \ref id is used.
    ///
    /// Key: "download"
    QString downloadPath;

    /// Parsed metadata for this plugin.
    ///
    /// Key: "meta"
    PluginMeta meta;

    /// Root URL for this plugin to append the filenames from the metadata to.
    ///
    /// Not present in JSON. Computed afterward.
    QUrl downloadURL;

    std::weak_ptr<PluginRepository> repository;
};

using RemotePluginPtr = std::shared_ptr<const RemotePlugin>;

}  // namespace chatterino

// Allow use in QVariant
Q_DECLARE_METATYPE(chatterino::RemotePluginPtr)

#endif
