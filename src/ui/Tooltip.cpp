#include "Tooltip.hpp"

#include <QLabel>
#include <QVBoxLayout>

#include "ab/Row.hpp"
#include "ab/util/MakeWidget.hpp"

#ifdef USEWINSDK
#    include <Windows.h>
#endif

namespace chatterino::ui
{
    Tooltip* Tooltip::instance()
    {
        static auto tooltip = new Tooltip();

        return tooltip;
    }

    Tooltip::Tooltip()
        : ab::BaseWindow(ab::BaseWindow::TopMost)
    {
        this->setStayInScreenRect(true);

        this->setAttribute(Qt::WA_ShowWithoutActivating);
        this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
                             Qt::X11BypassWindowManagerHint |
                             Qt::BypassWindowManagerHint);

        this->setCenterWidget(ab::makeWidget<QFrame>([&](QFrame* w) {
            w->setObjectName("tooltip-content");

            w->setLayout(ab::makeLayout<ab::Row>({
                ab::makeWidget<QLabel>([&](QLabel* w) {
                    this->label_ = w;
                    w->setAlignment(Qt::AlignHCenter);
                    w->setSizePolicy(
                        QSizePolicy::Maximum, QSizePolicy::Preferred);
                }),
            }));
        }));

        this->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

        assert(this->label_);
    }

#ifdef USEWINSDK
    void Tooltip::raise()
    {
        ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
#endif

    void Tooltip::setText(const QString& text)
    {
        this->label_->setText(text);
    }

    void Tooltip::setWordWrap(bool wrap)
    {
        this->label_->setWordWrap(wrap);
    }
}  // namespace chatterino::ui
