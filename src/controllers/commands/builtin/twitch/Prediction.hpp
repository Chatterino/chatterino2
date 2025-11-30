#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /prediction
QString createPrediction(const CommandContext &ctx);

/// /endprediction
QString endPrediction(const CommandContext &ctx);

/// /cancelprediction
QString cancelPrediction(const CommandContext &ctx);

}  // namespace chatterino::commands
