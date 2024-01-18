#pragma once

#include "common/Singleton.hpp"

#include <QtGlobal>

#ifdef CHATTERINO_WITH_CRASHPAD
#    include <client/crashpad_client.h>

#    include <memory>
#endif

namespace chatterino {

class Args;
class Paths;

class CrashHandler : public Singleton
{
    const Paths &paths;

public:
    explicit CrashHandler(const Paths &paths_);

    bool shouldRecover() const
    {
        return this->shouldRecover_;
    }

    /// Sets and saves whether Chatterino should restart on a crash
    void saveShouldRecover(bool value);

    void initialize(Settings &settings, const Paths &paths) override;

private:
    bool shouldRecover_ = false;
};

#ifdef CHATTERINO_WITH_CRASHPAD
std::unique_ptr<crashpad::CrashpadClient> installCrashHandler(
    const Args &args, const Paths &paths);
#endif

}  // namespace chatterino
