#include "Recovery.hpp"

#include "CommandLine.hpp"

#include <build/build_config.h>

#if BUILDFLAG(IS_WIN)
#    include "WinSupport.hpp"

#    include <util/win/command_line.h>
#endif

#include <minidump/minidump_user_extension_stream_data_source.h>
#include <snapshot/exception_snapshot.h>
#include <snapshot/process_snapshot.h>

#include <chrono>
#include <format>

#include <mini_chromium/base/strings/utf_string_conversions.h>

using namespace std::literals;
namespace chrono = std::chrono;

namespace {

/// Get a value out of a map if it exists.
template <class K, class V>
std::optional<V> tryGet(const std::map<K, V> &map, const K &key)
{
    auto it = map.find(key);
    if (it == map.end())
    {
        return std::nullopt;
    }
    return it->second;
}

chrono::utc_time<chrono::seconds> parseTime(const std::string &source)
{
    chrono::utc_time<chrono::seconds> parsed;
    std::stringstream(source) >> chrono::parse("%FT%TZ", parsed);
    return parsed;
}

}  // namespace

std::unique_ptr<crashpad::MinidumpUserExtensionStreamDataSource>
    CrashRecoverer::ProduceStreamData(crashpad::ProcessSnapshot *snapshot)
{
    if (snapshot == nullptr)
    {
        return {};
    }

    const auto &annotations = snapshot->AnnotationsSimpleMap();
    auto exePathOpt = tryGet(annotations, "exePath"s);
    auto canRestartOpt = tryGet(annotations, "canRestart"s);
    auto startedAtOpt = tryGet(annotations, "startedAt"s);

    if (!exePathOpt || !canRestartOpt || !startedAtOpt)
    {
        return {};
    }
    const auto &exePath = *exePathOpt;
    const auto &canRestart = *canRestartOpt;
    const auto &startedAt = *startedAtOpt;

    auto exeArguments = tryGet(annotations, "exeArguments"s);

    if (canRestart != "true"s)
    {
        return {};
    }

    auto startTime = parseTime(startedAt);
    auto now = std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::utc_clock::now());
    if (now - startTime < 1min)
    {
        return {};
    }

    // we know we can restart now,
    // prepare the arguments (gather basic info about the crash)

    std::vector<std::wstring> arguments;
    if (exeArguments)
    {
        arguments = splitEncodedChatterinoArgs(base::UTF8ToWide(*exeArguments));
    }

    const auto *exception = snapshot->Exception();
    if (exception != nullptr)
    {
        arguments.emplace_back(L"--cr-exception-code"s);
        arguments.emplace_back(std::format(L"{}", exception->Exception()));
#if BUILDFLAG(IS_WIN)
        auto message = formatCommonException(exception->Exception());
        if (message)
        {
            arguments.emplace_back(L"--cr-exception-message"s);
            arguments.emplace_back(*message);
        }
#endif
    }

    this->restartInfo_ = RestartInfo{
        .applicationPath = base::UTF8ToWide(exePath),
        .arguments = arguments,
    };

    return {};
}

void CrashRecoverer::attemptRecovery() const
{
#if BUILDFLAG(IS_WIN)
    if (!this->restartInfo_)
    {
        return;
    }
    const auto &restartInfo = *this->restartInfo_;

    auto commandline =
        std::format(L"\"{}\" --crash-recovery", restartInfo.applicationPath);
    for (const auto &arg : restartInfo.arguments)
    {
        crashpad::AppendCommandLineArgument(arg, &commandline);
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    auto result = CreateProcessW(
        restartInfo.applicationPath.c_str(),
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        const_cast<wchar_t *>(commandline.c_str()), nullptr, nullptr, FALSE,
        DETACHED_PROCESS, nullptr, nullptr, &si, &pi);

    if (result == TRUE)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#endif
}
