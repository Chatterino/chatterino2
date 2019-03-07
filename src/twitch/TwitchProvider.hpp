#pragma once

#include "Provider.hpp"

namespace chatterino
{
    class TwitchProviderData;

    class TwitchProvider final : public Provider
    {
    public:
        TwitchProvider();
        ~TwitchProvider() override;

        // ui
        const QString& name() override;
        const QString& title() override;

        virtual std::pair<QLayout*, std::function<RoomPtr()>>
            buildSelectChannelLayout(const QJsonObject& data) override;

        // rooms
        RoomPtr addRoom(const QJsonObject& data) override;

    private:
        TwitchProviderData* this_{};
    };
}  // namespace chatterino
