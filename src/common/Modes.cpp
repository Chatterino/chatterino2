#include "Modes.hpp"

#include "util/CombinePath.hpp"

#include <QCoreApplication>

namespace chatterino {

Modes::Modes()
{
    QFile file(combinePath(QCoreApplication::applicationDirPath(), "modes"));
    file.open(QIODevice::ReadOnly);

    while (!file.atEnd())
    {
        auto line = QString(file.readLine()).trimmed();

        // we need to know if it is a nightly build to disable updates on windows
        if (line == "nightly")
        {
            this->isNightly = true;
        }
        else if (line == "portable")
        {
            this->isPortable = true;
        }
    }
    this->isNightly = true;
    // there are no plans for "stable" Dankerino, Chatterino stable is always
    // very outdated and potentially less stable than nightly. Also disables
    // updates to make sure that Dankerino doesn't get overridden by upstream
    // Chatterino
}

const Modes &Modes::instance()
{
    static Modes instance;
    return instance;
}

}  // namespace chatterino
