#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

int jsonStringify(lua_State *L);

}  // namespace chatterino::lua::api

#endif
