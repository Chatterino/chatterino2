#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/table.hpp>

namespace chatterino::lua::api::qt {
// NOLINTBEGIN(readability-identifier-naming)

sol::object createModule(sol::state_view lua);

// NOLINTEND(readability-identifier-naming)
}  // namespace chatterino::lua::api::qt
#endif
