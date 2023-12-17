#pragma once

#include "common/Singleton.hpp"

#include <QtGlobal>

#ifdef CHATTERINO_WITH_CRASHPAD
#    include <client/crashpad_client.h>

#    include <memory>
#endif

namespace chatterino {

class CrashHandler : public Singleton
{
public:
    bool shouldRecover() const
    {
        return this->shouldRecover_;
    }
    void setShouldRecover(bool value);

    void initialize(Settings &settings, Paths &paths) override;

private:
    bool shouldRecover_ = false;
};

#ifdef CHATTERINO_WITH_CRASHPAD
std::unique_ptr<crashpad::CrashpadClient> installCrashHandler();
#endif

}  // namespace chatterino
