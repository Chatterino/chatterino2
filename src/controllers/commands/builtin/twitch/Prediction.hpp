#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /prediction
QString createPrediction(const CommandContext &ctx);

/// /lockprediction
QString lockPrediction(const CommandContext &ctx);

/// /cancelprediction
QString cancelPrediction(const CommandContext &ctx);

/// /completeprediction
QString completePrediction(const CommandContext &ctx);

}  // namespace chatterino::commands
