// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/spellcheck/SpellChecker.hpp"

#include "Test.hpp"

using namespace chatterino;
using namespace chatterino::spellcheck;
using namespace Qt::Literals;

TEST(SpellChecker, prettyLossyBcp47Description)
{
    std::vector<std::pair<QString, QStringView>> cases{
        {u""_s, u""},
        {u"index"_s, u"index"},
        {u"AF"_s, u"AF"},
        {u"af"_s, u"Afrikaans"},
        {u"ekk"_s, u"ekk"},
        {u"el-polyton"_s, u"Greek (polyton)"},
        {u"en-Latn-GB"_s, u"English (Latin, United Kingdom)"},
        {
            u"en-Latn-GB-variant"_s,
            u"English (Latin, United Kingdom, variant)",
        },
        {u"pt"_s, u"Portuguese"},
        {u"pt-PT"_s, u"Portuguese (Portugal)"},
        {u"sr"_s, u"Serbian"},
        {u"sr-Latn"_s, u"Serbian (Latin)"},
        {u"sv-FI"_s, u"Swedish (Finland)"},
        {u"tlh"_s, u"tlh"},
        {u"tlh-Latn"_s, u"tlh-Latn"},
        {u"tr"_s, u"Turkish"},
    };

    for (const auto &[bcp47, desc] : cases)
    {
        ASSERT_EQ(prettyLossyBcp47Description(bcp47), desc);
    }
}
