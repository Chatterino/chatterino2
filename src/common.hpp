#pragma once

#include <QString>
#include <boost/preprocessor.hpp>
#include <string>

#include "debug/log.hpp"

namespace chatterino {

enum class HighlightState {
    None,
    Highlighted,
    NewMessage,
};

inline QString qS(const std::string &string)
{
    return QString::fromStdString(string);
}

}  // namespace chatterino
