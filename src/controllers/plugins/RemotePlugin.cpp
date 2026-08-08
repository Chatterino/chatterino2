// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/RemotePlugin.hpp"

using namespace Qt::Literals;

namespace chatterino {

RemotePlugin::RemotePlugin(const QJsonObject &root)
    : id(root["id"_L1].toString())
    , downloadPath(root["download"_L1].toString())
    , meta(root["meta"_L1].toObject())
{
}

}  // namespace chatterino

#endif
