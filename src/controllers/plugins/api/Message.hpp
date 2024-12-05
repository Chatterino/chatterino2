#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "messages/Message.hpp"

#    include <sol/forward.hpp>

namespace chatterino::lua::api::message {

void createUserType(sol::table &c2);

}  // namespace chatterino::lua::api::message

#endif
