#pragma once

#include "common/QLogging.hpp"

#include <QNetworkProxy>
#include <websocketpp/error.hpp>

#include <string>

namespace chatterino {

class Env;

/** This class manipulates the global network configuration (e.g. proxies). */
class NetworkConfigurationProvider
{
public:
    /** This class should never be instantiated. */
    NetworkConfigurationProvider() = delete;

    /**
     * Applies the configuration requested from the environment variables.
     *
     * Currently a proxy is applied if configured.
     */
    static void applyFromEnv(const Env &env);

    static void applyToWebSocket(const auto &connection)
    {
        const auto applicationProxy = QNetworkProxy::applicationProxy();
        if (applicationProxy.type() != QNetworkProxy::HttpProxy)
        {
            return;
        }
        std::string url = "http://";
        url += applicationProxy.hostName().toStdString();
        url += ":";
        url += std::to_string(applicationProxy.port());
        websocketpp::lib::error_code ec;
        connection->set_proxy(url, ec);
        if (ec)
        {
            qCDebug(chatterinoNetwork)
                << "Couldn't set websocket proxy:" << ec.value();
            return;
        }

        connection->set_proxy_basic_auth(
            applicationProxy.user().toStdString(),
            applicationProxy.password().toStdString(), ec);
        if (ec)
        {
            qCDebug(chatterinoNetwork)
                << "Couldn't set websocket proxy auth:" << ec.value();
        }
    }
};

}  // namespace chatterino
