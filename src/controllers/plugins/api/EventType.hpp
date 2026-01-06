// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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
