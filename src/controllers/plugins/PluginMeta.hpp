// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/PluginPermission.hpp"

#    include <QJsonObject>
#    include <QString>
#    include <semver/semver.hpp>

#    include <vector>

namespace chatterino {

struct PluginMeta {
    // for more info on these fields see docs/plugin-info.schema.json

    // display name of the plugin
    QString name;

    // description shown to the user
    QString description;

    // plugin authors shown to the user
    std::vector<QString> authors;

    // license name
    QString license;

    // version of the plugin
    semver::version version;

    // optionally a homepage link
    QString homepage;

    // optionally tags that might help in searching for the plugin
    std::vector<QString> tags;

    std::vector<PluginPermission> permissions;

    /// Extra fields ignored by Chatterino but preserved when re-exporting the
    /// metadata.
    ///
    /// Key: "private"
    QJsonObject privateFields;

    /// The URL to the meta.json. For local plugins this is empty. If this URL
    /// is added in the settings, the plugin can be updated through the UI.
    ///
    /// Plugins with the same ID but from different base URLs are considered
    /// different (see #isRelatedTo()).
    ///
    /// Key: "remote"
    QString remoteBaseURL;

    /// Files that are included in this plugin.
    ///
    /// This is only used for downloading plugins. It's not used at runtime
    /// to check the integrity or file accesses. When serializing to JSON, this
    /// is discarded.
    ///
    /// Key: "files"
    std::vector<QString> files;

    // errors that occurred while parsing info.json
    std::vector<QString> errors;

    bool isValid() const
    {
        return this->errors.empty();
    }

    /// Is this meta from the same source as `remote`?
    ///
    /// Plugins are considered from the same source if they are installed from
    /// the same base URL.
    bool isRelatedTo(const PluginMeta &otherMeta,
                     QString *conflicts = nullptr) const;

    explicit PluginMeta(const QJsonObject &obj);
    // This is for tests
    PluginMeta() = default;

    QJsonObject toJson() const;
};

}  // namespace chatterino

#endif
