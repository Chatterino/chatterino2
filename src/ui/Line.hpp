#pragma once

#include "ab/BaseWidget.hpp"

namespace chatterino::ui
{
    /// A horizontal grey line widget.
    class Line : public ab::BaseWidget
    {
        Q_OBJECT

    public:
        void paintEvent(QPaintEvent*) override;
    };
}  // namespace chatterino::ui
