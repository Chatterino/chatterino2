#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

sol::object loadJson(sol::state_view lua);

}  // namespace chatterino::lua::api

#endif
