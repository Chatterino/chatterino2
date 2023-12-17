#include "widgets/splits/InputCompletionPopup.hpp"

#include "controllers/completion/sources/UserSource.hpp"
#include "controllers/completion/strategies/ClassicEmoteStrategy.hpp"
#include "controllers/completion/strategies/ClassicUserStrategy.hpp"
#include "controllers/completion/strategies/SmartEmoteStrategy.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/splits/InputCompletionItem.hpp"

namespace chatterino {

InputCompletionPopup::InputCompletionPopup(QWidget *parent)
    : BasePopup({BasePopup::EnableCustomFrame, BasePopup::Frameless,
                 BasePopup::DontFocus, BaseWindow::DisableLayoutSave},
                parent)
    , model_(this)
{
    this->initLayout();
    this->themeChangedEvent();

    QObject::connect(&this->redrawTimer_, &QTimer::timeout, this, [this] {
        if (this->isVisible())
        {
            this->ui_.listView->doItemsLayout();
        }
    });
    this->redrawTimer_.setInterval(33);
}

void InputCompletionPopup::updateCompletion(const QString &text,
                                            CompletionKind kind,
                                            ChannelPtr channel)
{
    if (this->currentKind_ != kind || this->currentChannel_ != channel)
    {
        // New completion context
        this->beginCompletion(kind, std::move(channel));
    }

    assert(this->model_.hasSource());
    this->model_.updateResults(text, MAX_ENTRY_COUNT);

    // Move selection to top row
    if (this->model_.rowCount() != 0)
    {
        this->ui_.listView->setCurrentIndex(this->model_.index(0));
    }
}

std::unique_ptr<completion::Source> InputCompletionPopup::getSource() const
{
    assert(this->currentChannel_ != nullptr);

    if (!this->currentKind_)
    {
        return nullptr;
    }

    // Currently, strategies are hard coded.
    switch (*this->currentKind_)
    {
        case CompletionKind::Emote:
            if (getSettings()->useSmartEmoteCompletion)
            {
                return std::make_unique<completion::EmoteSource>(
                    this->currentChannel_.get(),
                    std::make_unique<completion::SmartEmoteStrategy>(),
                    this->callback_);
            }
            return std::make_unique<completion::EmoteSource>(
                this->currentChannel_.get(),
                std::make_unique<completion::ClassicEmoteStrategy>(),
                this->callback_);
        case CompletionKind::User:
            return std::make_unique<completion::UserSource>(
                this->currentChannel_.get(),
                std::make_unique<completion::ClassicUserStrategy>(),
                this->callback_);
        default:
            return nullptr;
    }
}

void InputCompletionPopup::beginCompletion(CompletionKind kind,
                                           ChannelPtr channel)
{
    this->currentKind_ = kind;
    this->currentChannel_ = std::move(channel);
    this->model_.setSource(this->getSource());
}

void InputCompletionPopup::endCompletion()
{
    this->currentKind_ = std::nullopt;
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

void InputCompletionPopup::themeChangedEvent()
{
    BasePopup::themeChangedEvent();

    this->ui_.listView->refreshTheme(*getTheme());
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
