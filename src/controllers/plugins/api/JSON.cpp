#include "controllers/plugins/api/JSON.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/api/JSONParse.hpp"
#    include "controllers/plugins/api/JSONStringify.hpp"

#    include <sol/sol.hpp>

namespace chatterino::lua::api {

sol::object loadJson(sol::state_view lua)
{
    return lua.create_table_with(    //
        "parse", jsonParse,          //
        "stringify", jsonStringify,  //
        // pushed as lightuserdata
        "null", static_cast<void *>(nullptr)  //
    );
}

}  // namespace chatterino::lua::api

#endif
