#pragma once

#include "debug/log.hpp"

#include <QString>
#include <boost/preprocessor.hpp>

#include <string>

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
