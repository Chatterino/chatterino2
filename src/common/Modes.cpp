// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/Modes.hpp"

#include "common/Args.hpp"
#include "util/CombinePath.hpp"

#include <QCoreApplication>

namespace chatterino {

Modes::Modes(const Args &args)
{
    QString parentDirectory;

    if (args.portableDirectory.has_value())
    {
        this->isPortable = true;
        parentDirectory = args.portableDirectory.value();
    }
    else
    {
        parentDirectory = QCoreApplication::applicationDirPath();
    }

    QFile file(combinePath(parentDirectory, "modes"));

    if (!file.open(QIODevice::ReadOnly))
    {
        return;
    }

    while (!file.atEnd())
    {
        auto line = QString(file.readLine()).trimmed();

        if (line == "portable")
        {
            this->isPortable = true;
        }
        else if (line == "externally-packaged")
        {
            this->isExternallyPackaged = true;
        }
    }
}

}  // namespace chatterino
