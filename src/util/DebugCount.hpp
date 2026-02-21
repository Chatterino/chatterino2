// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/FlagsEnum.hpp"

#include <magic_enum/magic_enum.hpp>
#include <QString>

#include <cstddef>

namespace chatterino {

enum class DebugObject : size_t {
    // windowing
    AttachedWindow,
    BaseWindow,

    // live updates
    LiveUpdatesSubscription,
    LiveUpdatesSubscriptionBacklog,
    LiveUpdatesConnection,

    // http/other networking
    HTTPRequestStarted,
    HTTPRequestSuccess,
    NetworkData,

    // images
    Image,
    LoadedImage,
    AnimatedImage,
    BytesImageCurrent,
    BytesImageLoaded,
    BytesImageUnloaded,

    LastImageGcExpired,
    LastImageGcEligible,
    LastImageGcLeft,

    // Lua
    LuaHTTPResponse,
    LuaHTTPRequest,

    // Messages
    MessageDrawingBuffer,
    MessageElement,
    MessageLayout,
    MessageLayoutElement,
    MessageThread,
    Message,

    Count,
};

class DebugCount
{
public:
    static void set(DebugObject target, int64_t amount);

    static void increase(DebugObject target, int64_t amount);
    static void increase(DebugObject target)
    {
        DebugCount::increase(target, 1);
    }

    static void decrease(DebugObject target, int64_t amount);
    static void decrease(DebugObject target)
    {
        DebugCount::decrease(target, 1);
    }

    static QString getDebugText();
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<chatterino::DebugObject>(
        chatterino::DebugObject value) noexcept
{
    using chatterino::DebugObject;
    switch (value)
    {
        // class names
        case chatterino::DebugObject::AttachedWindow:
        case chatterino::DebugObject::BaseWindow:
        case chatterino::DebugObject::NetworkData:
        case chatterino::DebugObject::MessageElement:
        case chatterino::DebugObject::MessageLayout:
        case chatterino::DebugObject::MessageLayoutElement:
        case chatterino::DebugObject::MessageThread:
        case chatterino::DebugObject::Message:
        default:
            return default_tag;

        case chatterino::DebugObject::AnimatedImage:
            return "animated images";
        case chatterino::DebugObject::LiveUpdatesSubscription:
            return "LiveUpdates subscriptions";
        case chatterino::DebugObject::LiveUpdatesSubscriptionBacklog:
            return "LiveUpdates subscription backlog";
        case chatterino::DebugObject::LiveUpdatesConnection:
            return "LiveUpdates connections";
        case chatterino::DebugObject::HTTPRequestStarted:
            return "http requests started";
        case chatterino::DebugObject::HTTPRequestSuccess:
            return "http requests succeeded";
        case chatterino::DebugObject::Image:
            return "images";
        case chatterino::DebugObject::LoadedImage:
            return "loaded images";
        case chatterino::DebugObject::BytesImageCurrent:
            return "image bytes";
        case chatterino::DebugObject::BytesImageLoaded:
            return "image bytes (ever loaded)";
        case chatterino::DebugObject::BytesImageUnloaded:
            return "image bytes (ever unloaded)";
        case chatterino::DebugObject::LastImageGcExpired:
            return "last image gc: expired";
        case chatterino::DebugObject::LastImageGcEligible:
            return "last image gc: eligible";
        case chatterino::DebugObject::LastImageGcLeft:
            return "last image gc: left after gc";
        case chatterino::DebugObject::LuaHTTPResponse:
            return "lua::api::HTTPResponse";
        case chatterino::DebugObject::LuaHTTPRequest:
            return "lua::api::HTTPRequest";
        case chatterino::DebugObject::MessageDrawingBuffer:
            return "message drawing buffers";
    }
}
