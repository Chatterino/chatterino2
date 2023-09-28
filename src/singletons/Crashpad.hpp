#pragma once

#include "common/FlagsEnum.hpp"
#include "common/Singleton.hpp"

#include <QtGlobal>

#ifdef CHATTERINO_WITH_CRASHPAD
#    include <client/crashpad_client.h>

#    include <memory>
#endif

namespace chatterino {

class CrashRecovery : public Singleton
{
public:
    enum class Flag : qulonglong {
        None,
        DoCrashRecovery = 1 << 0,
    };

    using Flags = FlagsEnum<Flag>;

    Flags recoveryFlags() const;
    void updateFlags(Flags flags);

    void initialize(Settings &settings, Paths &paths) override;

private:
    Flags currentFlags_ = Flag::None;
};

#ifdef CHATTERINO_WITH_CRASHPAD
std::unique_ptr<crashpad::CrashpadClient> installCrashHandler();
#endif

}  // namespace chatterino
