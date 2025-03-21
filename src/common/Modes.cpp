#include "common/Modes.hpp"

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
        else if (line == "externally-packaged")
        {
            this->isExternallyPackaged = true;
        }
    }
}

const Modes &Modes::instance()
{
    static Modes instance;
    return instance;
}

}  // namespace chatterino
