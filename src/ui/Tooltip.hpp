#pragma once

#include "ab/BaseWindow.hpp"

class QLabel;

namespace chatterino::ui
{
    class Tooltip : public ab::BaseWindow
    {
        Q_OBJECT

    public:
        static Tooltip* instance();

        Tooltip();

        void setText(const QString& text);
        void setWordWrap(bool wrap);

#ifdef USEWINSDK
        void raise();
#endif

    private:
        QLabel* label_{};
    };
}  // namespace chatterino::ui
