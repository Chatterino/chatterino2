#pragma once

#include <QStringList>

namespace chatterino {

/// Command line arguments passed to Chatterino.
class Args
{
public:
    Args(const QStringList &args);

    bool printVersion{};
    bool crashRecovery{};
};

void initArgs(const QStringList &args);
const Args &getArgs();

}  // namespace chatterino
