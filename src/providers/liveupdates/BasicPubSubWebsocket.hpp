#pragma once

#include "providers/twitch/ChatterinoWebSocketppLogger.hpp"

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/logger/basic.hpp>

namespace chatterino {

struct BasicPubSubConfig : public websocketpp::config::asio_tls_client {
    // NOLINTBEGIN(modernize-use-using)
    typedef websocketpp::log::chatterinowebsocketpplogger<
        concurrency_type, websocketpp::log::elevel>
        elog_type;
    typedef websocketpp::log::chatterinowebsocketpplogger<
        concurrency_type, websocketpp::log::alevel>
        alog_type;

    struct PerMessageDeflateConfig {
    };

    typedef websocketpp::extensions::permessage_deflate::disabled<
        PerMessageDeflateConfig>
        permessage_deflate_type;
    // NOLINTEND(modernize-use-using)
};

namespace liveupdates {
    using WebsocketClient = websocketpp::client<chatterino::BasicPubSubConfig>;
    using WebsocketHandle = websocketpp::connection_hdl;
    using WebsocketErrorCode = websocketpp::lib::error_code;
}  // namespace liveupdates

}  // namespace chatterino
