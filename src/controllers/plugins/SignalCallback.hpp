// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/LuaFunctionRef.hpp"

namespace chatterino::lua {

struct SignalCallback : LuaFunctionRef {
    using LuaFunctionRef::LuaFunctionRef;

    void operator()(auto &&...args) const
    {
        this->tryCall<void>(u"SignalCallback::operator()",
                            std::forward<decltype(args)>(args)...);
    }
};

}  // namespace chatterino::lua

#endif
