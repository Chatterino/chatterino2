/*
 * Copyright (c) 2020, Peter Thorson, Steve Wills. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include "common/QLogging.hpp"

#include <websocketpp/common/cpp11.hpp>
#include <websocketpp/logger/basic.hpp>
#include <websocketpp/logger/levels.hpp>

#include <string>

namespace websocketpp::log {

template <typename concurrency, typename names>
class chatterinowebsocketpplogger : public basic<concurrency, names>
{
public:
    using base = chatterinowebsocketpplogger<concurrency, names>;

    chatterinowebsocketpplogger(channel_type_hint::value)
        : m_static_channels(0xffffffff)
        , m_dynamic_channels(0)
    {
    }

    chatterinowebsocketpplogger(std::ostream *)
        : m_static_channels(0xffffffff)
        , m_dynamic_channels(0)
    {
    }

    chatterinowebsocketpplogger(level c, channel_type_hint::value)
        : m_static_channels(c)
        , m_dynamic_channels(0)
    {
    }

    chatterinowebsocketpplogger(level c, std::ostream *)
        : m_static_channels(c)
        , m_dynamic_channels(0)
    {
    }

    ~chatterinowebsocketpplogger()
    {
    }

    chatterinowebsocketpplogger(
        chatterinowebsocketpplogger<concurrency, names> const &other)
        : m_static_channels(other.m_static_channels)
        , m_dynamic_channels(other.m_dynamic_channels)
    {
    }

#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    chatterinowebsocketpplogger<concurrency, names> &operator=(
        chatterinowebsocketpplogger<concurrency, names> const &) = delete;
#endif  // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#ifdef _WEBSOCKETPP_MOVE_SEMANTICS_
    /// Move constructor
    chatterinowebsocketpplogger(
        chatterinowebsocketpplogger<concurrency, names> &&other)
        : m_static_channels(other.m_static_channels)
        , m_dynamic_channels(other.m_dynamic_channels)
    {
    }
#    ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    // no move assignment operator because of const member variables
    chatterinowebsocketpplogger<concurrency, names> &operator=(
        chatterinowebsocketpplogger<concurrency, names> &&) = delete;
#    endif  // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
#endif      // _WEBSOCKETPP_MOVE_SEMANTICS_

    /// Explicitly do nothing, this logger doesn't support changing ostream
    void set_ostream(std::ostream *)
    {
    }

    /// Dynamically enable the given list of channels
    /**
     * @param channels The package of channels to enable
     */
    void set_channels(level channels)
    {
        if (channels == names::none)
        {
            clear_channels(names::all);
            return;
        }

        scoped_lock_type lock(m_lock);
        m_dynamic_channels |= (channels & m_static_channels);
    }

    /// Dynamically disable the given list of channels
    /**
     * @param channels The package of channels to disable
     */
    void clear_channels(level channels)
    {
        scoped_lock_type lock(m_lock);
        m_dynamic_channels &= ~channels;
    }

    /// Write a string message to the given channel
    /**
     * @param channel The channel to write to
     * @param msg The message to write
     */
    void write(level channel, std::string const &msg)
    {
        scoped_lock_type lock(m_lock);
        if (!this->dynamic_test(channel))
        {
            return;
        }
        qCDebug(chatterinoWebsocket).nospace()
            << names::channel_name(channel) << ": "
            << QString::fromStdString(msg);
    }

    /// Write a cstring message to the given channel
    /**
     * @param channel The channel to write to
     * @param msg The message to write
     */
    void write(level channel, char const *msg)
    {
        scoped_lock_type lock(m_lock);
        if (!this->dynamic_test(channel))
        {
            return;
        }
        qCDebug(chatterinoWebsocket).nospace()
            << names::channel_name(channel) << ": " << msg;
    }

    /// Test whether a channel is statically enabled
    /**
     * @param channel The package of channels to test
     */

    _WEBSOCKETPP_CONSTEXPR_TOKEN_ bool static_test(level channel) const
    {
        return ((channel & m_static_channels) != 0);
    }

    /// Test whether a channel is dynamically enabled
    /**
     * @param channel The package of channels to test
     */
    bool dynamic_test(level channel)
    {
        return ((channel & m_dynamic_channels) != 0);
    }

protected:
    using scoped_lock_type = typename concurrency::scoped_lock_type;
    using mutex_type = typename concurrency::mutex_type;
    mutex_type m_lock;

private:
    level const m_static_channels;
    level m_dynamic_channels;
};

}  // namespace websocketpp::log
