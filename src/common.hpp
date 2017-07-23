#pragma once

#include <QString>

#include <string>

namespace chatterino {

inline QString qS(const std::string &string)
{
    return QString::fromStdString(string);
}

}  // namespace chatterino
