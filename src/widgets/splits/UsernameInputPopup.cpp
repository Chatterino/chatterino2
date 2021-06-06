#include "UsernameInputPopup.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/listview/GenericListView.hpp"
#include "widgets/splits/UsernameInputItem.hpp"

namespace chatterino {

UsernameInputPopup::UsernameInputPopup(QWidget *parent)
    : BasePopup({BasePopup::EnableCustomFrame, BasePopup::Frameless,
                 BasePopup::DontFocus},
                parent)
    , model_(this)
{
    this->initLayout();

    QObject::connect(&this->redrawTimer_, &QTimer::timeout, this, [this] {
        if (this->isVisible())
            this->ui_.listView->doItemsLayout();
    });
    this->redrawTimer_.setInterval(33);
}

void UsernameInputPopup::initLayout()
{
    LayoutCreator creator = {this};

    auto listView =
        creator.emplace<GenericListView>().assign(&this->ui_.listView);
    listView->setInvokeActionOnTab(true);

    listView->setModel(&this->model_);
    QObject::connect(listView.getElement(), &GenericListView::closeRequested,
                     this, [this] {
                         this->close();
                     });
}

void UsernameInputPopup::updateUsers(const QString &text, ChannelPtr channel)
{
    auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());
    if (twitchChannel)
    {
        auto chatters = twitchChannel->accessChatters()->filterByPrefix(text);
        this->model_.clear();
        int count = 0;
        for (const auto &name : chatters)
        {
            this->model_.addItem(
                std::make_unique<UsernameInputItem>(name, this->callback_));

            if (count++ == maxUsernameCount)
                break;
        }
        if (!chatters.empty())
        {
            this->ui_.listView->setCurrentIndex(this->model_.index(0));
        }
    }
}
bool UsernameInputPopup::eventFilter(QObject *watched, QEvent *event)
{
    return this->ui_.listView->eventFilter(watched, event);
}

void UsernameInputPopup::setInputAction(ActionCallback callback)
{
    this->callback_ = std::move(callback);
}

void UsernameInputPopup::showEvent(QShowEvent *)
{
    this->redrawTimer_.start();
}

void UsernameInputPopup::hideEvent(QHideEvent *)
{
    this->redrawTimer_.stop();
}

}  // namespace chatterino
