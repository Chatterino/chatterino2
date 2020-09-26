#include "widgets/splits/SplitHeader.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/TooltipPreviewImage.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "util/LayoutHelper.hpp"
#include "widgets/Label.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/helper/CommonTexts.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QDesktopWidget>
#include <QDrag>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <cmath>

#ifdef USEWEBENGINE
#    include "widgets/StreamView.hpp"
#endif

namespace chatterino {
namespace {
    auto formatRoomMode(TwitchChannel &channel) -> QString
    {
        QString text;

        {
            auto modes = channel.accessRoomModes();

            if (modes->r9k)
                text += "r9k, ";
            if (modes->slowMode)
                text +=
                    QString("slow(%1), ").arg(QString::number(modes->slowMode));
            if (modes->emoteOnly)
                text += "emote, ";
            if (modes->submode)
                text += "sub, ";
            if (modes->followerOnly != -1)
            {
                if (modes->followerOnly != 0)
                {
                    text += QString("follow(%1m), ")
                                .arg(QString::number(modes->followerOnly));
                }
                else
                {
                    text += QString("follow, ");
                }
            }
        }

        if (text.length() > 2)
        {
            text = text.mid(0, text.size() - 2);
        }

        if (!text.isEmpty())
        {
            static QRegularExpression commaReplacement("^(.+?, .+?,) (.+)$");

            auto match = commaReplacement.match(text);
            if (match.hasMatch())
                text = match.captured(1) + '\n' + match.captured(2);
        }

        if (text.isEmpty() && channel.hasModRights())
            return "none";

        return text;
    }
    auto formatTooltip(const TwitchChannel::StreamStatus &s, QString thumbnail)
    {
        return QString("<style>.center { text-align: center; }</style> \
            <p class=\"center\">%1%2%3%4%5%6 for %7 with %8 viewers</p>")
            .arg(s.title.toHtmlEscaped())
            .arg(s.title.isEmpty() ? QString() : "<br><br>")
            .arg(getSettings()->thumbnailSizeStream.getValue() > 0
                     ? ((thumbnail.isEmpty()
                             ? "Couldn't fetch thumbnail"
                             : "<img src=\"data:image/jpg;base64, " +
                                   thumbnail + "\"/>") +
                        "<br>")
                     : QString())
            .arg(s.game.toHtmlEscaped())
            .arg(s.game.isEmpty() ? QString() : "<br>")
            .arg(s.rerun ? "Vod-casting" : "Live")
            .arg(getSettings()->hideViewerCountAndDuration ? "&lt;Hidden&gt;"
                                                           : s.uptime)
            .arg(getSettings()->hideViewerCountAndDuration
                     ? "&lt;Hidden&gt;"
                     : QString::number(s.viewerCount));
    }
    auto formatOfflineTooltip(const TwitchChannel::StreamStatus &s)
    {
        return QString("<style>.center { text-align: center; }</style> \
                       <p class=\"center\">Offline<br>%1</p>")
            .arg(s.title.toHtmlEscaped());
    }
    auto formatTitle(const TwitchChannel::StreamStatus &s, Settings &settings)
    {
        auto title = QString();

        // live
        if (s.rerun)
            title += " (rerun)";
        else if (s.streamType.isEmpty())
            title += " (" + s.streamType + ")";
        else
            title += " (live)";

        // description
        if (settings.headerUptime)
            title += " - " + s.uptime;
        if (settings.headerViewerCount)
            title += " - " + QString::number(s.viewerCount);
        if (settings.headerGame && !s.game.isEmpty())
            title += " - " + s.game;
        if (settings.headerStreamTitle && !s.title.isEmpty())
            title += " - " + s.title;

        return title;
    }
    auto distance(QPoint a, QPoint b)
    {
        auto x = std::abs(a.x() - b.x());
        auto y = std::abs(a.y() - b.y());

        return std::sqrt(x * x + y * y);
    }
}  // namespace

SplitHeader::SplitHeader(Split *_split)
    : BaseWidget(_split)
    , split_(_split)
{
    this->initializeLayout();

    this->setMouseTracking(true);
    this->updateChannelText();
    this->handleChannelChanged();
    this->updateModerationModeIcon();

    this->split_->focused.connect([this]() { this->themeChangedEvent(); });
    this->split_->focusLost.connect([this]() { this->themeChangedEvent(); });
    this->split_->channelChanged.connect(
        [this]() { this->handleChannelChanged(); });

    this->managedConnect(getApp()->accounts->twitch.currentUserChanged,
                         [this] { this->updateModerationModeIcon(); });

    auto _ = [this](const auto &, const auto &) { this->updateChannelText(); };
    getSettings()->headerViewerCount.connect(_, this->managedConnections_);
    getSettings()->headerStreamTitle.connect(_, this->managedConnections_);
    getSettings()->headerGame.connect(_, this->managedConnections_);
    getSettings()->headerUptime.connect(_, this->managedConnections_);
}

void SplitHeader::initializeLayout()
{
    auto layout = makeLayout<QHBoxLayout>({
        // space
        makeWidget<BaseWidget>(
            [](auto w) { w->setScaleIndependantSize(8, 4); }),
        // title
        this->titleLabel_ = makeWidget<Label>([](auto w) {
            w->setSizePolicy(QSizePolicy::MinimumExpanding,
                             QSizePolicy::Preferred);
            w->setCentered(true);
            w->setHasOffset(false);
        }),
        // space
        makeWidget<BaseWidget>(
            [](auto w) { w->setScaleIndependantSize(8, 4); }),
        // mode
        this->modeButton_ = makeWidget<EffectLabel>([&](auto w) {
            w->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            w->hide();
            this->initializeModeSignals(*w);
            w->setMenu(this->createChatModeMenu());
        }),
        // moderator
        this->moderationButton_ = makeWidget<Button>([&](auto w) {
            QObject::connect(
                w, &Button::clicked, this,
                [this, w](Qt::MouseButton button) mutable {
                    switch (button)
                    {
                        case Qt::LeftButton:
                            if (getSettings()->moderationActions.empty())
                            {
                                getApp()->windows->showSettingsDialog(
                                    SettingsDialogPreference::
                                        ModerationActions);
                                this->split_->setModerationMode(true);
                            }
                            else
                            {
                                auto moderationMode =
                                    this->split_->getModerationMode();

                                this->split_->setModerationMode(
                                    !moderationMode);
                                w->setDim(Button::Dim(moderationMode));
                            }
                            break;

                        case Qt::RightButton:
                        case Qt::MiddleButton:
                            getApp()->windows->showSettingsDialog(
                                SettingsDialogPreference::ModerationActions);
                            break;
                    }
                });
        }),
        // dropdown
        this->dropdownButton_ = makeWidget<Button>([&](auto w) {
            /// XXX: this never gets disconnected
            this->split_->channelChanged.connect([this] {
                this->dropdownButton_->setMenu(this->createMainMenu());
            });
        }),
        // add split
        this->addButton_ = makeWidget<Button>([&](auto w) {
            w->setPixmap(getResources().buttons.addSplitDark);
            w->setEnableMargin(false);

            QObject::connect(w, &Button::leftClicked, this,
                             [this]() { this->split_->addSibling(); });
        }),
    });

    // update moderation button when items changed
    this->managedConnect(getSettings()->moderationActions.delayedItemsChanged,
                         [this] {
                             if (getSettings()->moderationActions.empty())
                             {
                                 if (this->split_->getModerationMode())
                                     this->split_->setModerationMode(true);
                             }
                             else
                             {
                                 if (this->split_->getModerationMode())
                                     this->split_->setModerationMode(true);
                             }
                         });

    getSettings()->customURIScheme.connect([this] {
        if (const auto drop = this->dropdownButton_)
        {
            drop->setMenu(this->createMainMenu());
        }
    });

    layout->setMargin(0);
    layout->setSpacing(0);
    this->setLayout(layout);

    this->setAddButtonVisible(false);
}

std::unique_ptr<QMenu> SplitHeader::createMainMenu()
{
    // top level menu
    auto menu = std::make_unique<QMenu>();
    menu->addAction("Change channel", this->split_, &Split::changeChannel,
                    QKeySequence("Ctrl+R"));
    menu->addAction("Close", this->split_, &Split::deleteFromContainer,
                    QKeySequence("Ctrl+W"));
    menu->addSeparator();
    menu->addAction("Popup", this->split_, &Split::popup,
                    QKeySequence("Ctrl+N"));
    menu->addAction("Search", this->split_, &Split::showSearch,
                    QKeySequence("Ctrl+F"));
    menu->addSeparator();
#ifdef USEWEBENGINE
    this->dropdownMenu.addAction("Start watching", this, [this] {
        ChannelPtr _channel = this->split->getChannel();
        TwitchChannel *tc = dynamic_cast<TwitchChannel *>(_channel.get());

        if (tc != nullptr)
        {
            StreamView *view = new StreamView(
                _channel,
                "https://player.twitch.tv/?parent=twitch.tv&channel=" +
                    tc->name);
            view->setAttribute(Qt::WA_DeleteOnClose, true);
            view->show();
        }
    });
#endif

    if (dynamic_cast<TwitchChannel *>(this->split_->getChannel().get()))
    {
        menu->addAction(OPEN_IN_BROWSER, this->split_, &Split::openInBrowser);
#ifndef USEWEBENGINE
        menu->addAction(OPEN_PLAYER_IN_BROWSER, this->split_,
                        &Split::openBrowserPlayer);
#endif
        menu->addAction(OPEN_IN_STREAMLINK, this->split_,
                        &Split::openInStreamlink);

        if (!getSettings()->customURIScheme.getValue().isEmpty())
        {
            menu->addAction("Open in custom player", this->split_,
                            &Split::openWithCustomScheme);
        }
        menu->addSeparator();
    }

    if (this->split_->getChannel()->getType() == Channel::Type::TwitchWhispers)
    {
        menu->addAction(OPEN_WHISPERS_IN_BROWSER, this->split_,
                        &Split::openWhispersInBrowser);
        menu->addSeparator();
    }

    // reload / reconnect
    if (this->split_->getChannel()->canReconnect())
        menu->addAction("Reconnect", this, SLOT(reconnect()));

    if (dynamic_cast<TwitchChannel *>(this->split_->getChannel().get()))
    {
        menu->addAction("Reload channel emotes", this,
                        SLOT(reloadChannelEmotes()), QKeySequence("F5"));
        menu->addAction("Reload subscriber emotes", this,
                        SLOT(reloadSubscriberEmotes()), QKeySequence("F5"));
    }

    menu->addSeparator();

    {
        // "How to..." sub menu
        auto subMenu = new QMenu("How to...", this);
        subMenu->addAction("move split", this->split_, &Split::explainMoving);
        subMenu->addAction("add/split", this->split_, &Split::explainSplitting);
        menu->addMenu(subMenu);
    }

    menu->addSeparator();

    // sub menu
    auto moreMenu = new QMenu("More", this);

    moreMenu->addAction("Toggle moderation mode", this->split_, [this]() {
        this->split_->setModerationMode(!this->split_->getModerationMode());
    });

    if (this->split_->getChannel()->getType() == Channel::Type::TwitchMentions)
    {
        auto action = new QAction(this);
        action->setText("Enable /mention tab highlights");
        action->setCheckable(true);

        QObject::connect(moreMenu, &QMenu::aboutToShow, this, [action, this]() {
            action->setChecked(getSettings()->highlightMentions);
        });
        action->connect(action, &QAction::triggered, this, [this]() {
            getSettings()->highlightMentions =
                !getSettings()->highlightMentions;
        });

        moreMenu->addAction(action);
    }

    if (dynamic_cast<TwitchChannel *>(this->split_->getChannel().get()))
    {
        moreMenu->addAction("Show viewer list", this->split_,
                            &Split::showViewerList);

        moreMenu->addAction("Subscribe", this->split_, &Split::openSubPage);

        auto action = new QAction(this);
        action->setText("Notify when live");
        action->setCheckable(true);

        QObject::connect(moreMenu, &QMenu::aboutToShow, this, [action, this]() {
            action->setChecked(getApp()->notifications->isChannelNotified(
                this->split_->getChannel()->getName(), Platform::Twitch));
        });
        action->connect(action, &QAction::triggered, this, [this]() {
            getApp()->notifications->updateChannelNotification(
                this->split_->getChannel()->getName(), Platform::Twitch);
        });

        moreMenu->addAction(action);
    }

    if (dynamic_cast<TwitchChannel *>(this->split_->getChannel().get()))
    {
        auto action = new QAction(this);
        action->setText("Mute highlight sound");
        action->setCheckable(true);

        QObject::connect(moreMenu, &QMenu::aboutToShow, this, [action, this]() {
            action->setChecked(getSettings()->isMutedChannel(
                this->split_->getChannel()->getName()));
        });
        action->connect(action, &QAction::triggered, this, [this]() {
            getSettings()->toggleMutedChannel(
                this->split_->getChannel()->getName());
        });

        moreMenu->addAction(action);
    }

    moreMenu->addSeparator();
    moreMenu->addAction("Clear messages", this->split_, &Split::clear);
    //    moreMenu->addSeparator();
    //    moreMenu->addAction("Show changelog", this,
    //    SLOT(moreMenuShowChangelog()));
    menu->addMenu(moreMenu);

    return menu;
}

std::unique_ptr<QMenu> SplitHeader::createChatModeMenu()
{
    auto menu = std::make_unique<QMenu>();

    auto setSub = new QAction("Subscriber only", this);
    auto setEmote = new QAction("Emote only", this);
    auto setSlow = new QAction("Slow", this);
    auto setR9k = new QAction("R9K", this);
    auto setFollowers = new QAction("Followers only", this);

    setFollowers->setCheckable(true);
    setSub->setCheckable(true);
    setEmote->setCheckable(true);
    setSlow->setCheckable(true);
    setR9k->setCheckable(true);

    menu->addAction(setEmote);
    menu->addAction(setSub);
    menu->addAction(setSlow);
    menu->addAction(setR9k);
    menu->addAction(setFollowers);

    this->managedConnections_.push_back(this->modeUpdateRequested_.connect(  //
        [this, setSub, setEmote, setSlow, setR9k, setFollowers]() {
            auto twitchChannel =
                dynamic_cast<TwitchChannel *>(this->split_->getChannel().get());
            if (twitchChannel == nullptr)
            {
                this->modeButton_->hide();
                return;
            }

            auto roomModes = twitchChannel->accessRoomModes();

            setR9k->setChecked(roomModes->r9k);
            setSlow->setChecked(roomModes->slowMode);
            setEmote->setChecked(roomModes->emoteOnly);
            setSub->setChecked(roomModes->submode);
            setFollowers->setChecked(roomModes->followerOnly != -1);
        }));

    auto toggle = [this](const QString &command, QAction *action) mutable {
        this->split_->getChannel().get()->sendMessage(
            command + (action->isChecked() ? "" : "off"));
        action->setChecked(!action->isChecked());
    };

    QObject::connect(
        setSub, &QAction::triggered, this,
        [setSub, toggle]() mutable { toggle("/subscribers", setSub); });

    QObject::connect(
        setEmote, &QAction::triggered, this,
        [setEmote, toggle]() mutable { toggle("/emoteonly", setEmote); });

    QObject::connect(setSlow, &QAction::triggered, this, [setSlow, this]() {
        if (!setSlow->isChecked())
        {
            this->split_->getChannel().get()->sendMessage("/slowoff");
            setSlow->setChecked(false);
            return;
        };
        auto ok = bool();
        auto seconds = QInputDialog::getInt(this, "", "Seconds:", 10, 0, 500, 1,
                                            &ok, Qt::FramelessWindowHint);
        if (ok)
        {
            this->split_->getChannel().get()->sendMessage(
                QString("/slow %1").arg(seconds));
        }
        else
        {
            setSlow->setChecked(false);
        }
    });

    QObject::connect(
        setFollowers, &QAction::triggered, this, [setFollowers, this]() {
            if (!setFollowers->isChecked())
            {
                this->split_->getChannel().get()->sendMessage("/followersoff");
                setFollowers->setChecked(false);
                return;
            };
            auto ok = bool();
            auto time = QInputDialog::getText(
                this, "", "Time:", QLineEdit::Normal, "15m", &ok,
                Qt::FramelessWindowHint,
                Qt::ImhLowercaseOnly | Qt::ImhPreferNumbers);
            if (ok)
            {
                this->split_->getChannel().get()->sendMessage(
                    QString("/followers %1").arg(time));
            }
            else
            {
                setFollowers->setChecked(false);
            }
        });

    QObject::connect(
        setR9k, &QAction::triggered, this,
        [setR9k, toggle]() mutable { toggle("/r9kbeta", setR9k); });

    return menu;
}

void SplitHeader::updateRoomModes()
{
    this->modeUpdateRequested_.invoke();
}

void SplitHeader::initializeModeSignals(EffectLabel &label)
{
    this->modeUpdateRequested_.connect([this, &label] {
        if (auto twitchChannel = dynamic_cast<TwitchChannel *>(
                this->split_->getChannel().get()))  //
        {
            label.setEnable(twitchChannel->hasModRights());

            // set the label text
            auto text = formatRoomMode(*twitchChannel);

            if (!text.isEmpty())
            {
                label.getLabel().setText(text);
                label.show();
                return;
            }
        }

        label.hide();
    });
}

void SplitHeader::handleChannelChanged()
{
    this->channelConnections_.clear();

    auto channel = this->split_->getChannel();
    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        this->channelConnections_.emplace_back(
            twitchChannel->liveStatusChanged.connect(
                [this]() { this->updateChannelText(); }));
    }
}

void SplitHeader::scaleChangedEvent(float scale)
{
    int w = int(28 * scale);

    this->setFixedHeight(w);
    this->dropdownButton_->setFixedWidth(w);
    this->moderationButton_->setFixedWidth(w);
    this->addButton_->setFixedWidth(w * 5 / 8);
}

void SplitHeader::setAddButtonVisible(bool value)
{
    this->addButton_->setVisible(value);
}

void SplitHeader::updateChannelText()
{
    auto indirectChannel = this->split_->getIndirectChannel();
    auto channel = this->split_->getChannel();
    this->isLive_ = false;
    this->tooltipText_ = QString();

    auto title = channel->getName();

    if (indirectChannel.getType() == Channel::Type::TwitchWatching)
        title = "watching: " + (title.isEmpty() ? "none" : title);

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        const auto streamStatus = twitchChannel->accessStreamStatus();

        if (streamStatus->live)
        {
            this->isLive_ = true;
            // XXX: This URL format can be figured out from the Helix Get Streams API which we parse in TwitchChannel::parseLiveStatus
            QString url = "https://static-cdn.jtvnw.net/"
                          "previews-ttv/live_user_" +
                          channel->getName().toLower();
            switch (getSettings()->thumbnailSizeStream.getValue())
            {
                case 1:
                    url.append("-80x45.jpg");
                    break;
                case 2:
                    url.append("-160x90.jpg");
                    break;
                case 3:
                    url.append("-360x180.jpg");
                    break;
                default:
                    url = "";
            }
            if (!url.isEmpty() &&
                (!this->lastThumbnail_.isValid() ||
                 this->lastThumbnail_.elapsed() > 5 * 60 * 1000))
            {
                NetworkRequest(url, NetworkRequestType::Get)
                    .onSuccess([this](auto result) -> Outcome {
                        // NOTE: We do not follow the redirects, so we need to make sure we only treat code 200 as a valid image
                        if (result.status() == 200)
                        {
                            this->thumbnail_ = QString::fromLatin1(
                                result.getData().toBase64());
                        }
                        else
                        {
                            this->thumbnail_.clear();
                        }
                        this->updateChannelText();
                        return Success;
                    })
                    .execute();
                this->lastThumbnail_.restart();
            }
            this->tooltipText_ = formatTooltip(*streamStatus, this->thumbnail_);
            title += formatTitle(*streamStatus, *getSettings());
        }
        else
        {
            this->tooltipText_ = formatOfflineTooltip(*streamStatus);
        }
    }

    this->titleLabel_->setText(title.isEmpty() ? "<empty>" : title);
}

void SplitHeader::updateModerationModeIcon()
{
    auto moderationMode = this->split_->getModerationMode() &&
                          !getSettings()->moderationActions.empty();

    this->moderationButton_->setPixmap(
        moderationMode ? getResources().buttons.modModeEnabled
                       : getResources().buttons.modModeDisabled);

    auto channel = this->split_->getChannel();
    auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel != nullptr &&
        (twitchChannel->hasModRights() || moderationMode))
    {
        this->moderationButton_->show();
    }
    else
    {
        this->moderationButton_->hide();
    }
}

void SplitHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), this->theme->splits.header.background);
    painter.setPen(this->theme->splits.header.border);
    painter.drawRect(0, 0, width() - 1, height() - 2);
    painter.fillRect(0, height() - 1, width(), 1,
                     this->theme->splits.background);
}

void SplitHeader::mousePressEvent(QMouseEvent *event)
{
    switch (event->button())
    {
        case Qt::LeftButton: {
            this->dragging_ = true;

            this->dragStart_ = event->pos();
        }
        break;

        case Qt::RightButton: {
            auto menu = this->createMainMenu().release();
            menu->setAttribute(Qt::WA_DeleteOnClose);
            menu->popup(this->mapToGlobal(event->pos() + QPoint(0, 4)));
        }
        break;
    }

    this->doubleClicked_ = false;
}

void SplitHeader::mouseReleaseEvent(QMouseEvent *event)
{
    this->dragging_ = false;
}

void SplitHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (this->dragging_)
    {
        if (distance(this->dragStart_, event->pos()) > 15 * this->scale())
        {
            this->split_->drag();
            this->dragging_ = false;
        }
    }
}

void SplitHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        this->split_->changeChannel();
    }
    this->doubleClicked_ = true;
}

void SplitHeader::enterEvent(QEvent *event)
{
    if (!this->tooltipText_.isEmpty())
    {
        auto channel = this->split_->getChannel().get();
        if (channel->getType() == Channel::Type::Twitch)
        {
            dynamic_cast<TwitchChannel *>(channel)->refreshTitle();
        }

        TooltipPreviewImage::instance().setImage(nullptr);

        auto tooltip = TooltipWidget::instance();
        tooltip->setText(this->tooltipText_);
        tooltip->setWordWrap(true);
        tooltip->adjustSize();
        auto pos = this->mapToGlobal(this->rect().bottomLeft()) +
                   QPoint((this->width() - tooltip->width()) / 2, 1);

        tooltip->moveTo(this, pos, false);
        tooltip->show();
        tooltip->raise();
    }

    BaseWidget::enterEvent(event);
}

void SplitHeader::leaveEvent(QEvent *event)
{
    TooltipWidget::instance()->hide();

    BaseWidget::leaveEvent(event);
}

void SplitHeader::themeChangedEvent()
{
    auto palette = QPalette();

    if (this->split_->hasFocus())
    {
        palette.setColor(QPalette::Foreground,
                         this->theme->splits.header.focusedText);
    }
    else
    {
        palette.setColor(QPalette::Foreground, this->theme->splits.header.text);
    }
    this->titleLabel_->setPalette(palette);

    // --
    if (this->theme->isLightTheme())
    {
        this->dropdownButton_->setPixmap(getResources().buttons.menuDark);
        this->addButton_->setPixmap(getResources().buttons.addSplit);
    }
    else
    {
        this->dropdownButton_->setPixmap(getResources().buttons.menuLight);
        this->addButton_->setPixmap(getResources().buttons.addSplitDark);
    }
}

void SplitHeader::moveSplit()
{
}

void SplitHeader::reloadChannelEmotes()
{
    auto channel = this->split_->getChannel();

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        twitchChannel->refreshFFZChannelEmotes(true);
        twitchChannel->refreshBTTVChannelEmotes(true);
    }
}

void SplitHeader::reloadSubscriberEmotes()
{
    getApp()->accounts->twitch.getCurrent()->loadEmotes();
}

void SplitHeader::reconnect()
{
    this->split_->getChannel()->reconnect();
}

}  // namespace chatterino
