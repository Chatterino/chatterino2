#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "common/ProviderId.hpp"

#include <QString>
#include <QWidget>
#include <boost/optional.hpp>
#include <boost/preprocessor.hpp>

#include <string>

namespace chatterino
{
    enum class HighlightState {
        None,
        Highlighted,
        NewMessage,
    };

    inline QString qS(const std::string& string)
    {
        return QString::fromStdString(string);
    }

    const Qt::KeyboardModifiers showSplitOverlayModifiers =
        Qt::ControlModifier | Qt::AltModifier;
    const Qt::KeyboardModifiers showAddSplitRegions =
        Qt::ControlModifier | Qt::AltModifier;
    const Qt::KeyboardModifiers showResizeHandlesModifiers =
        Qt::ControlModifier;

    static const char* ANONYMOUS_USERNAME_LABEL ATTR_UNUSED = " - anonymous - ";

    template <typename T>
    std::weak_ptr<T> weakOf(T* element)
    {
        return element->shared_from_this();
    }

    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    enum class CopyMode {
        Everything,
        OnlyTextAndEmotes,
    };

}  // namespace chatterino
