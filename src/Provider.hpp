#pragma once

#include "Room.hpp"

#include <functional>

class QString;
class QLayout;

namespace chatterino
{
    /*class SelectChannelPage
    {
        SelectChannelPage(const QJsonObject& data);
        RoomPtr getRoom();
    };*/

    class Provider
    {
    public:
        Provider();
        virtual ~Provider() = default;

        virtual const QString& name() = 0;
        virtual const QString& title() = 0;
        virtual std::pair<QLayout*, std::function<RoomPtr()>>
            buildSelectChannelLayout(const QJsonObject& data) = 0;

        virtual RoomPtr addRoom(const QJsonObject& data) = 0;
    };
}  // namespace chatterino
