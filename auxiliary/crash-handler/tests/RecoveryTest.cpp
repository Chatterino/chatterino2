#include "Recovery.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <minidump/minidump_user_extension_stream_data_source.h>
#include <snapshot/exception_snapshot.h>
#include <snapshot/process_snapshot.h>
#include <snapshot/unloaded_module_snapshot.h>
#include <util/win/exception_codes.h>

#include <chrono>

using namespace std::string_literals;
using namespace std::chrono_literals;
using namespace testing;

namespace chrono = std::chrono;

namespace {

class MockSnapshot : public crashpad::ProcessSnapshot
{
public:
    MOCK_METHOD(crashpad::ProcessID, ProcessID, (), (const, override));
    MOCK_METHOD(crashpad::ProcessID, ParentProcessID, (), (const, override));
    MOCK_METHOD(void, SnapshotTime, (timeval *), (const, override));
    MOCK_METHOD(void, ProcessStartTime, (timeval *), (const, override));
    MOCK_METHOD(void, ProcessCPUTimes, (timeval *, timeval *),
                (const, override));
    MOCK_METHOD(void, ReportID, (crashpad::UUID *), (const, override));
    MOCK_METHOD(void, ClientID, (crashpad::UUID *), (const, override));
    MOCK_METHOD((const std::map<std::string, std::string> &),
                AnnotationsSimpleMap, (), (const, override));
    MOCK_METHOD(const crashpad::SystemSnapshot *, System, (),
                (const, override));
    MOCK_METHOD(std::vector<const crashpad::ModuleSnapshot *>, Modules, (),
                (const, override));
    MOCK_METHOD(std::vector<crashpad::UnloadedModuleSnapshot>, UnloadedModules,
                (), (const, override));
    MOCK_METHOD(std::vector<const crashpad::ThreadSnapshot *>, Threads, (),
                (const, override));
    MOCK_METHOD(const crashpad::ExceptionSnapshot *, Exception, (),
                (const, override));
    MOCK_METHOD(std::vector<const crashpad::MemoryMapRegionSnapshot *>,
                MemoryMap, (), (const, override));
    MOCK_METHOD(std::vector<crashpad::HandleSnapshot>, Handles, (),
                (const, override));
    MOCK_METHOD(std::vector<const crashpad::MemorySnapshot *>, ExtraMemory, (),
                (const, override));
    MOCK_METHOD(const crashpad::ProcessMemory *, Memory, (), (const, override));
};

class MyExceptionSnapshot : public crashpad::ExceptionSnapshot
{
public:
    MyExceptionSnapshot(uint32_t exception)
        : exception_(exception)
    {
    }

    uint32_t Exception() const override
    {
        return this->exception_;
    }

    MOCK_METHOD(const crashpad::CPUContext *, Context, (), (const, override));
    MOCK_METHOD(uint64_t, ThreadID, (), (const, override));
    MOCK_METHOD(uint32_t, ExceptionInfo, (), (const, override));
    MOCK_METHOD(uint64_t, ExceptionAddress, (), (const, override));
    MOCK_METHOD(std::vector<const crashpad::MemorySnapshot *>, ExtraMemory, (),
                (const, override));
    MOCK_METHOD(const std::vector<uint64_t> &, Codes, (), (const, override));

private:
    uint32_t exception_;
};

std::string formatTs(chrono::utc_time<chrono::seconds> ts)
{
    return std::format("{:%FT%TZ}", ts);
}

std::string formatFromNow(chrono::seconds diff)
{
    return formatTs(
        chrono::time_point_cast<chrono::seconds>(chrono::utc_clock::now()) +
        diff);
}

}  // namespace

TEST(CrashRecoverer, ProduceStreamData)
{
    struct TestCase {
        const char *name;
        std::map<std::string, std::string> annotations;
        std::optional<MyExceptionSnapshot> exception;
        std::optional<RestartInfo> output;
    };

    std::initializer_list<TestCase> testCases{
        {
            "Restart + No Exception",
            {
                {"exePath"s, "foobar"s},
                {"canRestart"s, "true"s},
                {"startedAt"s, formatFromNow(-2min)},
            },
            std::nullopt,
            RestartInfo{
                L"foobar"s,
                {},
            },
        },
        {
            "Restart + Exception(access violation)",
            {
                {"exePath"s, "foobar"s},
                {"canRestart"s, "true"s},
                {"startedAt"s, formatFromNow(-2min)},
            },
            {EXCEPTION_ACCESS_VIOLATION},
            RestartInfo{
                L"foobar"s,
                {
                    L"--cr-exception-code"s,
                    std::format(L"{}", EXCEPTION_ACCESS_VIOLATION),
                    L"--cr-exception-message"s,
                    L"ExceptionAccessViolation"s,
                },
            },
        },
        {
            "Restart + Exception(user triggered) + Args",
            {
                {"exePath"s, "foobar"s},
                {"canRestart"s, "true"s},
                {"startedAt"s, formatFromNow(-2min)},
                {"exeArguments"s, "--foo+--bar=foo++bar"s},
            },
            {static_cast<uint32_t>(
                crashpad::ExceptionCodes::kTriggeredExceptionCode)},
            RestartInfo{
                L"foobar"s,
                {
                    L"--foo"s,
                    L"--bar=foo+bar"s,
                    L"--cr-exception-code"s,
                    std::format(
                        L"{}",
                        static_cast<uint32_t>(
                            crashpad::ExceptionCodes::kTriggeredExceptionCode)),
                    L"--cr-exception-message"s,
                    L"TriggeredExceptionCode"s,
                },
            },
        },
        {
            "No Restart + Exception(access violation)",
            {
                {"exePath"s, "foobar"s},
                {"canRestart"s, "false"s},
                {"startedAt"s, formatFromNow(-2min)},
            },
            {EXCEPTION_ACCESS_VIOLATION},
            std::nullopt,
        },
        {
            "No Restart + Args",
            {
                {"exePath"s, "foobar"s},
                {"canRestart"s, "false"s},
                {"startedAt"s, formatFromNow(-2min)},
                {"exeArguments"s, "--foo+--bar"s},
            },
            std::nullopt,
            std::nullopt,
        },
        {
            "No Restart",
            {
                {"exePath"s, "foobar"s},
                {"canRestart"s, "True"s},
                {"startedAt"s, formatFromNow(-2min)},
            },
            std::nullopt,
            std::nullopt,
        },
        {
            "Too early crash",
            {
                {"exePath"s, "foobar"s},
                {"canRestart"s, "true"s},
                {"startedAt"s, formatFromNow(-10s)},
                {"exeArguments"s, "--foo+--bar"s},
            },
            std::nullopt,
            std::nullopt,
        },
        {
            "Missing path",
            {
                {"canRestart"s, "true"s},
                {"startedAt"s, formatFromNow(-2min)},
                {"exeArguments"s, "--foo+--bar"s},
            },
            std::nullopt,
            std::nullopt,
        },
        {
            "Missing restart",
            {
                {"exePath"s, "foobar"s},
                {"startedAt"s, formatFromNow(-2min)},
                {"exeArguments"s, "--foo+--bar"s},
            },
            std::nullopt,
            std::nullopt,
        },
        {
            "Missing start",
            {
                {"exePath"s, "foobar"s},
                {"canRestart"s, "true"s},
                {"exeArguments"s, "--foo+--bar"s},
            },
            std::nullopt,
            std::nullopt,
        },
    };

    EXPECT_EQ(CrashRecoverer{}.ProduceStreamData(nullptr), nullptr);

    for (const auto &testCase : testCases)
    {
        CrashRecoverer recoverer;
        MockSnapshot snapshot;
        EXPECT_CALL(snapshot, AnnotationsSimpleMap())
            .Times(Exactly(1))
            .WillOnce(ReturnRef(testCase.annotations));

        EXPECT_CALL(snapshot, Exception())
            .Times(Exactly(testCase.output ? 1 : 0))
            .WillOnce(Invoke([&]() -> const crashpad::ExceptionSnapshot * {
                if (testCase.exception)
                {
                    return &*testCase.exception;
                }
                return nullptr;
            }));
        EXPECT_EQ(recoverer.ProduceStreamData(&snapshot), nullptr);
        EXPECT_EQ(recoverer.restartInfo(), testCase.output) << testCase.name;
    }
}
