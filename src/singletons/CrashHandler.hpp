#pragma once

#include "common/Singleton.hpp"

#include <QtGlobal>

#ifdef CHATTERINO_WITH_CRASHPAD
#    include <client/crashpad_client.h>

#    include <memory>
#endif

namespace chatterino {

class Args;

class CrashHandler : public Singleton
{
public:
    bool shouldRecover() const
    {
        return this->shouldRecover_;
    }

    /// Sets and saves whether Chatterino should restart on a crash
    void saveShouldRecover(bool value);

    void initialize(Settings &settings, Paths &paths) override;

private:
    bool shouldRecover_ = false;
};

#ifdef CHATTERINO_WITH_CRASHPAD
std::unique_ptr<crashpad::CrashpadClient> installCrashHandler(const Args &args);
#endif

}  // namespace chatterino
