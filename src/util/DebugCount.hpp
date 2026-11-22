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

    // window layout
    WindowLayoutSave,
    WindowLayoutSaveQueued,
    WindowLayoutSaveError,

    // settings
    SettingsSave,
    SettingsSaveSkipped,
    SettingsSaveError,

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
        case DebugObject::AttachedWindow:
        case DebugObject::BaseWindow:
        case DebugObject::NetworkData:
        case DebugObject::MessageElement:
        case DebugObject::MessageLayout:
        case DebugObject::MessageLayoutElement:
        case DebugObject::MessageThread:
        case DebugObject::Message:
        default:
            return default_tag;

        case DebugObject::AnimatedImage:
            return "animated images";
        case DebugObject::LiveUpdatesSubscription:
            return "LiveUpdates subscriptions";
        case DebugObject::LiveUpdatesSubscriptionBacklog:
            return "LiveUpdates subscription backlog";
        case DebugObject::LiveUpdatesConnection:
            return "LiveUpdates connections";
        case DebugObject::HTTPRequestStarted:
            return "http requests started";
        case DebugObject::HTTPRequestSuccess:
            return "http requests succeeded";
        case DebugObject::Image:
            return "images";
        case DebugObject::LoadedImage:
            return "loaded images";
        case DebugObject::BytesImageCurrent:
            return "image bytes";
        case DebugObject::BytesImageLoaded:
            return "image bytes (ever loaded)";
        case DebugObject::BytesImageUnloaded:
            return "image bytes (ever unloaded)";
        case DebugObject::LastImageGcExpired:
            return "last image gc: expired";
        case DebugObject::LastImageGcEligible:
            return "last image gc: eligible";
        case DebugObject::LastImageGcLeft:
            return "last image gc: left after gc";
        case DebugObject::LuaHTTPResponse:
            return "lua::api::HTTPResponse";
        case DebugObject::LuaHTTPRequest:
            return "lua::api::HTTPRequest";
        case DebugObject::MessageDrawingBuffer:
            return "message drawing buffers";

        case DebugObject::WindowLayoutSave:
            return "window layout save";
        case DebugObject::WindowLayoutSaveQueued:
            return "window layout save queued";
        case DebugObject::WindowLayoutSaveError:
            return "window layout save error";

        case DebugObject::SettingsSave:
            return "settings save";
        case DebugObject::SettingsSaveSkipped:
            return "settings save skipepd";
        case DebugObject::SettingsSaveError:
            return "settings save error";
    }
}
