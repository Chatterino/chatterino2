#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

int jsonParse(lua_State *L);

}  // namespace chatterino::lua::api

#endif
