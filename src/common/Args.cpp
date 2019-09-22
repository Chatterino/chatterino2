#include "Args.hpp"

namespace chatterino {

Args::Args(const QStringList &args)
{
    for (auto &&arg : args)
    {
        if (arg == "--crash-recovery")
        {
            this->crashRecovery = true;
        }
    }
}

static Args *instance = nullptr;

void initArgs(const QStringList &args)
{
    instance = new Args(args);
}

const Args &getArgs()
{
    assert(instance);

    return *instance;
}

}  // namespace chatterino
