#pragma once

#include "providers/liveupdates/BasicPubSubClient.hpp"
// this needs to be included for the specialization
// of std::hash for Subscription
#include "providers/seventv/eventapi/Subscription.hpp"

#include <QPointer>

namespace chatterino {
class SeventvEventAPI;

}  // namespace chatterino

namespace chatterino::seventv::eventapi {

struct Dispatch;
struct CosmeticCreateDispatch;
struct EntitlementCreateDeleteDispatch;

class Client : public BasicPubSubClient<Subscription>,
               std::enable_shared_from_this<Client>
{
public:
    Client(SeventvEventAPI &manager,
           std::chrono::milliseconds heartbeatInterval);

    void onOpen() /* override */;
    void onMessage(const QByteArray &msg) /* override */;

    std::chrono::milliseconds heartbeatInterval() const;
    void checkHeartbeat();

private:
    void handleDispatch(const Dispatch &dispatch);

    void onEmoteSetUpdate(const Dispatch &dispatch);
    void onUserUpdate(const Dispatch &dispatch);
    void onCosmeticCreate(const CosmeticCreateDispatch &cosmetic);
    void onEntitlementCreate(
        const EntitlementCreateDeleteDispatch &entitlement);
    void onEntitlementDelete(
        const EntitlementCreateDeleteDispatch &entitlement);

    std::atomic<std::chrono::time_point<std::chrono::steady_clock>>
        lastHeartbeat_;
    std::atomic<std::chrono::milliseconds> heartbeatInterval_;
    SeventvEventAPI &manager_;
};

}  // namespace chatterino::seventv::eventapi
