#include "widgets/helper/TitlebarButtons.hpp"

#ifdef USEWINSDK

#    include "widgets/helper/TitlebarButton.hpp"

#    include <Windows.h>

#    include <cassert>

namespace chatterino {

TitleBarButtons::TitleBarButtons(QWidget *window, TitleBarButton *minButton,
                                 TitleBarButton *maxButton,
                                 TitleBarButton *closeButton)
    : QObject(window)
    , window_(window)
    , minButton_(minButton)
    , maxButton_(maxButton)
    , closeButton_(closeButton)
{
}

void TitleBarButtons::hover(size_t ht, QPoint at)
{
    TitleBarButton *hovered{};
    TitleBarButton *other1{};
    TitleBarButton *other2{};
    switch (ht)
    {
        case HTMAXBUTTON:
            hovered = this->maxButton_;
            other1 = this->minButton_;
            other2 = this->closeButton_;
            break;
        case HTMINBUTTON:
            hovered = this->minButton_;
            other1 = this->maxButton_;
            other2 = this->closeButton_;
            break;
        case HTCLOSE:
            hovered = this->closeButton_;
            other1 = this->minButton_;
            other2 = this->maxButton_;
            break;
        default:
            assert(false && "TitleBarButtons::hover precondition violated");
            return;
    }
    hovered->ncEnter();
    hovered->ncMove(hovered->mapFromGlobal(at));
    other1->ncLeave();
    other2->ncLeave();
}

void TitleBarButtons::leave()
{
    this->minButton_->ncLeave();
    this->maxButton_->ncLeave();
    this->closeButton_->ncLeave();
}

void TitleBarButtons::mousePress(size_t ht, QPoint at)
{
    auto *button = this->buttonForHt(ht);
    button->ncMousePress(button->mapFromGlobal(at));
}

void TitleBarButtons::mouseRelease(size_t ht, QPoint at)
{
    auto *button = this->buttonForHt(ht);
    button->ncMouseRelease(button->mapFromGlobal(at));
}

void TitleBarButtons::updateMaxButton()
{
    this->maxButton_->setButtonStyle(
        this->window_->windowState().testFlag(Qt::WindowMaximized)
            ? TitleBarButtonStyle::Unmaximize
            : TitleBarButtonStyle::Maximize);
}

void TitleBarButtons::setSmallSize()
{
    this->minButton_->setScaleIndependantSize(30, 30);
    this->maxButton_->setScaleIndependantSize(30, 30);
    this->closeButton_->setScaleIndependantSize(30, 30);
}

void TitleBarButtons::setRegularSize()
{
    this->minButton_->setScaleIndependantSize(46, 30);
    this->maxButton_->setScaleIndependantSize(46, 30);
    this->closeButton_->setScaleIndependantSize(46, 30);
}

TitleBarButton *TitleBarButtons::buttonForHt(size_t ht) const
{
    switch (ht)
    {
        case HTMAXBUTTON:
            return this->maxButton_;
        case HTMINBUTTON:
            return this->minButton_;
        case HTCLOSE:
            return this->closeButton_;
        default:
            assert(false &&
                   "TitleBarButtons::buttonForHt precondition violated");
            return nullptr;
    }
}

}  // namespace chatterino

#endif
