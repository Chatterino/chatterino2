#include "InputCompletionPopup.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Emote.hpp"
#include "providers/autocomplete/AutocompleteEmoteSource.hpp"
#include "providers/autocomplete/AutocompleteEmoteStrategies.hpp"
#include "providers/autocomplete/AutocompleteUsersSource.hpp"
#include "providers/autocomplete/AutocompleteUserStrategies.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Emotes.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/listview/GenericListView.hpp"
#include "widgets/splits/InputCompletionItem.hpp"

namespace chatterino {

InputCompletionPopup::InputCompletionPopup(QWidget *parent)
    : BasePopup({BasePopup::EnableCustomFrame, BasePopup::Frameless,
                 BasePopup::DontFocus, BaseWindow::DisableLayoutSave},
                parent)
    , model_(this)
{
    this->initLayout();

    QObject::connect(&this->redrawTimer_, &QTimer::timeout, this, [this] {
        if (this->isVisible())
        {
            this->ui_.listView->doItemsLayout();
        }
    });
    this->redrawTimer_.setInterval(33);
}

void InputCompletionPopup::updateCompletion(const QString &text,
                                            InputCompletionMode mode,
                                            ChannelPtr channel)
{
    if (this->currentMode_ != mode || this->currentChannel_ != channel)
    {
        // New completion context
        this->beginCompletion(mode, std::move(channel));
    }

    assert(this->model_.hasSource());
    this->model_.updateResults(text, MAX_ENTRY_COUNT);

    // Move selection to top row
    if (this->model_.rowCount() != 0)
    {
        this->ui_.listView->setCurrentIndex(this->model_.index(0));
    }
}

std::unique_ptr<AutocompleteSource> InputCompletionPopup::getSource() const
{
    assert(this->currentMode_ != InputCompletionMode::None);
    assert(this->currentChannel_ != nullptr);

    // Currently, strategies are hard coded.
    switch (this->currentMode_)
    {
        case InputCompletionMode::Emote:
            return std::make_unique<AutocompleteEmoteSource>(
                this->currentChannel_, this->callback_,
                std::make_unique<ClassicAutocompleteEmoteStrategy>());
        case InputCompletionMode::User:
            return std::make_unique<AutocompleteUsersSource>(
                this->currentChannel_, this->callback_,
                std::make_unique<ClassicAutocompleteUserStrategy>());
        case InputCompletionMode::None:
            // unreachable due to assert, not using `default` to require exhaustiveness
            return nullptr;
    }
}

void InputCompletionPopup::beginCompletion(InputCompletionMode mode,
                                           ChannelPtr channel)
{
    this->currentMode_ = mode;
    this->currentChannel_ = std::move(channel);
    this->model_.setSource(this->getSource());
}

void InputCompletionPopup::endCompletion()
{
    this->currentMode_ = InputCompletionMode::None;
    this->currentChannel_ = nullptr;
    this->model_.setSource(nullptr);
}

void InputCompletionPopup::setInputAction(ActionCallback callback)
{
    this->callback_ = std::move(callback);
}

bool InputCompletionPopup::eventFilter(QObject *watched, QEvent *event)
{
    return this->ui_.listView->eventFilter(watched, event);
}

void InputCompletionPopup::showEvent(QShowEvent * /*event*/)
{
    this->redrawTimer_.start();
}

void InputCompletionPopup::hideEvent(QHideEvent * /*event*/)
{
    this->redrawTimer_.stop();
    this->endCompletion();
}

void InputCompletionPopup::initLayout()
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

}  // namespace chatterino
