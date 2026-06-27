// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

#include <cstdint>
#include <optional>

namespace chatterino {

namespace env {

constexpr const char *LOG_TO_FILE = "CHATTERINO_LOG_TO_FILE";

}  // namespace env

class Env
{
    Env();

public:
    static const Env &get();

    const QString recentMessagesApiUrl;
    const QString linkResolverUrl;
    const QString twitchServerHost;
    const uint16_t twitchServerPort;
    const bool twitchServerSecure;
    const std::optional<QString> proxyUrl;

    /// Log output from the application to a file at the given path
    const QString logToFile;
};

}  // namespace chatterino
