#include "util/FunctionEventFilter.hpp"

namespace chatterino {

FunctionEventFilter::FunctionEventFilter(
    QObject *parent, std::function<bool(QObject *, QEvent *)> function)
    : QObject(parent)
    , function_(std::move(function))
{
}

bool FunctionEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    return this->function_(watched, event);
}

}  // namespace chatterino
