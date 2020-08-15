#include "EmoteInputPopup.hpp"

#include "Application.hpp"
#include "messages/Emote.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Emotes.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/listview/GenericListView.hpp"
#include "widgets/splits/EmoteInputItem.hpp"

namespace chatterino {
namespace {

    struct _Emote {
        EmotePtr emote;
        QString providerName;
    };

    void addEmotes(std::vector<_Emote> &out, const EmoteMap &map,
                   const QString &text, const QString &providerName)
    {
        for (auto &&emote : map)
            if (emote.first.string.contains(text, Qt::CaseInsensitive))
                out.push_back({emote.second, providerName});
    }
}  // namespace

EmoteInputPopup::EmoteInputPopup(QWidget *parent)
    : BasePopup({BasePopup::EnableCustomFrame, BasePopup::Frameless,
                 BasePopup::DontFocus},
                parent)
    , model_(this)
{
    this->initLayout();

    //    this->connections_.addConnection(
    //        getApp()->emotes->gifTimer.signal.connect([this] {
    //            if (this->isVisible())
    //            {
    //                // redraw listview somehow
    //            }
    //        }));
}

void EmoteInputPopup::initLayout()
{
    LayoutCreator creator = {this};

    auto listView =
        creator.emplace<GenericListView>().assign(&this->ui_.listView);

    listView->setModel(&this->model_);
    QObject::connect(listView.getElement(), &GenericListView::closeRequested,
                     this, [this] { this->close(); });
}

void EmoteInputPopup::updateEmotes(const QString &text, ChannelPtr channel)
{
    std::vector<_Emote> emotes;

    if (auto tc = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        // TODO extract "Channel BetterTTV" text into a #define.
        if (auto bttv = tc->bttvEmotes())
            addEmotes(emotes, *bttv, text, "Channel BetterTTV");
        if (auto ffz = tc->ffzEmotes())
            addEmotes(emotes, *ffz, text, "Channel FrankerFaceZ");

        if (auto bttvG = tc->globalBttv().emotes())
            addEmotes(emotes, *bttvG, text, "Global BetterTTV");
        if (auto ffzG = tc->globalFfz().emotes())
            addEmotes(emotes, *ffzG, text, "Global FrankerFaceZ");
    }

    this->model_.clear();

    int count = 0;
    for (auto &&emote : emotes)
    {
        this->model_.addItem(std::make_unique<EmoteInputItem>(
            emote.emote, emote.emote->name.string + " - " + emote.providerName,
            this->callback_));

        if (count++ == maxLineCount)
            break;
    }

    if (!emotes.empty())
    {
        this->ui_.listView->setCurrentIndex(this->model_.index(0));
    }
}

bool EmoteInputPopup::eventFilter(QObject *watched, QEvent *event)
{
    return this->ui_.listView->eventFilter(watched, event);
}

void EmoteInputPopup::setInputAction(ActionCallback callback)
{
    this->callback_ = std::move(callback);
}

}  // namespace chatterino
