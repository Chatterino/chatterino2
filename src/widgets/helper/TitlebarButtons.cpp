#include "widgets/helper/TitlebarButtons.hpp"

#ifdef USEWINSDK

#    include "widgets/helper/TitlebarButton.hpp"

#    include <Windows.h>

namespace chatterino {

TitleBarButtons::TitleBarButtons(QWidget *window, TitleBarButton *minButton,
                                 TitleBarButton *maxButton,
                                 TitleBarButton *closeButton)
    : window_(window)
    , minButton_(minButton)
    , maxButton_(maxButton)
    , closeButton_(closeButton)
{
}

void TitleBarButtons::hover(size_t ht, QPoint at)
{
    auto [button, others] = this->buttonForHt(ht);
    button->ncEnter();
    button->ncMove(button->mapFromGlobal(at));
    for (auto *other : others)
    {
        other->ncLeave();
    }
}

void TitleBarButtons::leave()
{
    this->minButton_->ncLeave();
    this->maxButton_->ncLeave();
    this->closeButton_->ncLeave();
}

void TitleBarButtons::mouseDown(size_t ht, QPoint at)
{
    auto [button, others] = this->buttonForHt(ht);
    button->ncMouseDown(button->mapFromGlobal(at));
}

void TitleBarButtons::mouseUp(size_t ht, QPoint at)
{
    auto [button, others] = this->buttonForHt(ht);
    button->ncMouseUp(button->mapFromGlobal(at));
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

std::pair<TitleBarButton *, std::array<TitleBarButton *, 2>>
    TitleBarButtons::buttonForHt(size_t ht) const
{
    switch (ht)
    {
        case HTMAXBUTTON:
            return {this->maxButton_, {this->minButton_, this->closeButton_}};
        case HTMINBUTTON:
            return {this->minButton_, {this->maxButton_, this->closeButton_}};
        case HTCLOSE:
            return {this->closeButton_, {this->minButton_, this->maxButton_}};
        default:
            Q_ASSERT_X(false, Q_FUNC_INFO, "No button for hittest value found");
            return {this->closeButton_,
                    {this->minButton_, this->maxButton_}};  // fallback
    }
}

}  // namespace chatterino

#endif
