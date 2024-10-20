#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

namespace chatterino::lua::api {

/**
 * @exposeenum c2.EventType
 */
enum class EventType {
    CompletionRequested,
};

}  // namespace chatterino::lua::api
#endif
