// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

QString setLoggingRules(const CommandContext &ctx);

QString toggleThemeReload(const CommandContext &ctx);

QString listEnvironmentVariables(const CommandContext &ctx);

QString listArgs(const CommandContext &ctx);

QString forceImageGarbageCollection(const CommandContext &ctx);

QString forceImageUnload(const CommandContext &ctx);

QString forceLayoutChannelViews(const CommandContext &ctx);

QString incrementImageGeneration(const CommandContext &ctx);

QString invalidateBuffers(const CommandContext &ctx);

QString eventsub(const CommandContext &ctx);

QString debugTest(const CommandContext &ctx);

}  // namespace chatterino::commands
