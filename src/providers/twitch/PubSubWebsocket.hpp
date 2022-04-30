#pragma once

#include "providers/twitch/ChatterinoWebSocketppLogger.hpp"

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/logger/basic.hpp>

namespace chatterino {

struct chatterinoconfig : public websocketpp::config::asio_tls_client {
    typedef websocketpp::log::chatterinowebsocketpplogger<
        concurrency_type, websocketpp::log::elevel>
        elog_type;
    typedef websocketpp::log::chatterinowebsocketpplogger<
        concurrency_type, websocketpp::log::alevel>
        alog_type;

    struct permessage_deflate_config {
    };

    typedef websocketpp::extensions::permessage_deflate::disabled<
        permessage_deflate_config>
        permessage_deflate_type;
};

using WebsocketClient = websocketpp::client<chatterinoconfig>;
using WebsocketHandle = websocketpp::connection_hdl;
using WebsocketErrorCode = websocketpp::lib::error_code;

}  // namespace chatterino
