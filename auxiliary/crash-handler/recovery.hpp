#pragma once

#include <handler/user_stream_data_source.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct RestartInfo {
    std::wstring applicationPath;
    std::vector<std::wstring> arguments;

    auto operator<=>(const RestartInfo &) const = default;
};

class CrashRecoverer : public crashpad::UserStreamDataSource
{
public:
    std::unique_ptr<crashpad::MinidumpUserExtensionStreamDataSource>
        ProduceStreamData(crashpad::ProcessSnapshot *process_snapshot) override;

    void attemptRecovery() const;

    const std::optional<RestartInfo> &restartInfo() const
    {
        return this->restartInfo_;
    }

private:
    std::optional<RestartInfo> restartInfo_;
};
