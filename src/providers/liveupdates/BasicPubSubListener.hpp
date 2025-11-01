#pragma once

#include "common/websockets/WebSocketPool.hpp"
#include "util/PostToThread.hpp"

#include <QByteArray>
#include <QPointer>

#include <memory>

namespace chatterino {

template <typename Manager>
struct BasicPubSubListener : public WebSocketListener {
    BasicPubSubListener(std::weak_ptr<typename Manager::Client> client,
                        QPointer<Manager> manager, size_t id)
        : client(std::move(client))
        , manager(std::move(manager))
        , id(id)
    {
    }

    void onOpen() override
    {
        runInGuiThread([manager = this->manager, id = this->id] {
            if (manager)
            {
                manager->onConnectionOpen(id);
            }
        });
    }

    void onTextMessage(QByteArray msg) override
    {
        auto sp = this->client.lock();
        if (sp)
        {
            sp->onMessage(msg);
        }
    }

    void onBinaryMessage(QByteArray msg) override
    {
        auto sp = this->client.lock();
        if (sp)
        {
            sp->onMessage(msg);
        }
    }

    void onClose(std::unique_ptr<WebSocketListener> /* self */) override
    {
        runInGuiThread([manager = this->manager, id = this->id] {
            if (manager)
            {
                manager->onConnectionClose(id);
            }
        });
    }

    std::weak_ptr<typename Manager::Client> client;
    QPointer<Manager> manager;
    size_t id;
};

}  // namespace chatterino
