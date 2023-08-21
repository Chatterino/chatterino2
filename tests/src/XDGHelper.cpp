#include "util/XDGHelper.hpp"

#include <gtest/gtest.h>
#include <QDebug>

using namespace chatterino;

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

TEST(XDGHelper, ParseDesktopExecProgram)
{
    struct TestCase {
        QString input;
        QString expectedOutput;
    };

    std::vector<TestCase> testCases{
        {
            // Sanity check: Ensure simple Exec lines aren't made messed with
            "firefox",
            "firefox",
        },
        {
            // Simple trim after the first space
            "/usr/lib/firefox/firefox %u",
            "/usr/lib/firefox/firefox",
        },
        {
            // Simple unquote
            "\"/usr/lib/firefox/firefox\"",
            "/usr/lib/firefox/firefox",
        },
        {
            // Unquote + trim
            "\"/usr/lib/firefox/firefox\" %u",
            "/usr/lib/firefox/firefox",
        },
        {
            // Test malformed exec key (only one quote)
            "\"/usr/lib/firefox/firefox",
            "/usr/lib/firefox/firefox",
        },
        {
            // Quoted executable name with space
            "\"/usr/bin/my cool browser\"",
            "/usr/bin/my cool browser",
        },
        {
            // Executable name with reserved character
            "/usr/bin/\\$hadowwizardmoneybrowser",
            "/usr/bin/$hadowwizardmoneybrowser",
        },
    };

    for (const auto &test : testCases)
    {
        auto output = parseDesktopExecProgram(test.input);

        EXPECT_EQ(output, test.expectedOutput)
            << "Input '" << test.input.toStdString() << "' failed. Expected '"
            << test.expectedOutput.toStdString() << "' but got '"
            << output.toStdString() << "'";
    }
}

#endif
