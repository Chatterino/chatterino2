// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/InvalidHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"

namespace chatterino::highlights {

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool InvalidHighlight::isValid()
{
    return false;
}

HighlightCheck InvalidHighlight::buildCheck() const
{
    return {};
}

QDebug operator<<(QDebug dbg, const InvalidHighlight & /*v*/)
{
    dbg.nospace() << "InvalidHighlight()";

    return dbg;
}

}  // namespace chatterino::highlights
