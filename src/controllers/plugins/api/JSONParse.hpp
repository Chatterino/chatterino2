#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include <sol/forward.hpp>

namespace chatterino::lua::api {

/**
 * Parse a string as JSON
 *
 * @lua@param input string The text to parse
 * @lua@param opts? {allow_comments?: boolean, allow_trailing_commas?: boolean} Additional options for parsing. `allow_comments` will allow C++ style comments ('// text').
 * @lua@return any
 * @exposed c2.json_parse
 */
int jsonParse(lua_State *L);

}  // namespace chatterino::lua::api

#endif
