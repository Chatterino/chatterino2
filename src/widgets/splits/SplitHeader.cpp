#include "widgets/splits/SplitHeader.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Label.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QByteArray>
#include <QDesktopWidget>
#include <QDrag>
#include <QInputDialog>
#include <QMenu>
#include <QMimeData>
#include <QPainter>

#ifdef USEWEBENGINE
#include "widgets/StreamView.hpp"
#endif

namespace chatterino {

SplitHeader::SplitHeader(Split *_split)
    : BaseWidget(_split)
    , split_(_split)
{
    this->split_->focused.connect([this]() { this->themeChangedEvent(); });
    this->split_->focusLost.connect([this]() { this->themeChangedEvent(); });

    auto app = getApp();

    LayoutCreator<SplitHeader> layoutCreator(this);
    auto layout = layoutCreator.emplace<QHBoxLayout>().withoutMargin();
    layout->setSpacing(0);
    {
        // channel name label
        auto title = layout.emplace<Label>().assign(&this->titleLabel);
        title->setSizePolicy(QSizePolicy::MinimumExpanding,
                             QSizePolicy::Preferred);
        title->setCentered(true);
        title->setHasOffset(false);

        // mode button
        auto mode = layout.emplace<RippleEffectLabel>(nullptr).assign(
            &this->modeButton_);

        mode->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        mode->hide();

        this->setupModeLabel(*mode);

        mode->setMenu(this->createChatModeMenu());

        // moderation mode
        auto moderator = layout.emplace<RippleEffectButton>(this).assign(
            &this->moderationButton_);

        QObject::connect(
            moderator.getElement(), &RippleEffectButton::clicked, this,
            [this, moderator]() mutable {
                this->split_->setModerationMode(
                    !this->split_->getModerationMode());

                moderator->setDim(!this->split_->getModerationMode());
            });

        this->updateModerationModeIcon();

        // dropdown label
        auto dropdown = layout.emplace<RippleEffectButton>(this).assign(
            &this->dropdownButton_);
        dropdown->setMouseTracking(true);
        //        dropdown->setPixmap(*app->resources->splitHeaderContext->getPixmap());
        //        dropdown->setScaleIndependantSize(23, 23);
        dropdown->setMenu(this->createMainMenu());
        QObject::connect(dropdown.getElement(),
                         &RippleEffectButton::leftMousePress, this, [this] {});
    }

    // ---- misc
    this->layout()->setMargin(0);
    this->scaleChangedEvent(this->getScale());

    this->updateChannelText();

    this->initializeChannelSignals();

    this->split_->channelChanged.connect([this]() {
        this->initializeChannelSignals();  //
    });

    this->managedConnect(app->accounts->twitch.currentUserChanged,
                         [this] { this->updateModerationModeIcon(); });

    this->setMouseTracking(true);

    // Update title on title-settings-change
    getSettings()->showViewerCount.connect(
        [this](const auto &, const auto &) { this->updateChannelText(); },
        this->managedConnections_);
    getSettings()->showTitle.connect(
        [this](const auto &, const auto &) { this->updateChannelText(); },
        this->managedConnections_);
    getSettings()->showGame.connect(
        [this](const auto &, const auto &) { this->updateChannelText(); },
        this->managedConnections_);
    getSettings()->showUptime.connect(
        [this](const auto &, const auto &) { this->updateChannelText(); },
        this->managedConnections_);
}

SplitHeader::~SplitHeader()
{
    this->onlineStatusChangedConnection_.disconnect();
}

std::unique_ptr<QMenu> SplitHeader::createMainMenu()
{
    auto menu = std::make_unique<QMenu>();
    menu->addAction("New split", this->split_, &Split::doAddSplit,
                    QKeySequence(tr("Ctrl+T")));
    menu->addAction("Close split", this->split_, &Split::doCloseSplit,
                    QKeySequence(tr("Ctrl+W")));
    menu->addAction("Change channel", this->split_, &Split::doChangeChannel,
                    QKeySequence(tr("Ctrl+R")));
    menu->addSeparator();
    menu->addAction("Viewer list", this->split_, &Split::showViewerList);
    menu->addAction("Search", this->split_, &Split::showSearchPopup,
                    QKeySequence(tr("Ctrl+F")));
    menu->addSeparator();
    menu->addAction("Popup", this->split_, &Split::doPopup);
#ifdef USEWEBENGINE
    this->dropdownMenu.addAction("Start watching", this, [this] {
        ChannelPtr _channel = this->split->getChannel();
        TwitchChannel *tc = dynamic_cast<TwitchChannel *>(_channel.get());

        if (tc != nullptr) {
            StreamView *view = new StreamView(
                _channel, "https://player.twitch.tv/?channel=" + tc->name);
            view->setAttribute(Qt::WA_DeleteOnClose, true);
            view->show();
        }
    });
#endif
    menu->addAction("Open in browser", this->split_, &Split::openInBrowser);
#ifndef USEWEBENGINE
    menu->addAction("Open player in browser", this->split_,
                    &Split::openInPopupPlayer);
#endif
    menu->addAction("Open streamlink", this->split_, &Split::openInStreamlink);

    auto action = new QAction(this);
    action->setText("Notify when live");
    action->setCheckable(true);

    QObject::connect(menu.get(), &QMenu::aboutToShow, this, [action, this]() {
        int i = 0;
        action->setChecked(getApp()->notifications->isChannelNotified(
            this->split_->getChannel()->getName(), i));
    });
    action->connect(action, &QAction::triggered, this, [this]() {
        int i = 0;
        getApp()->notifications->updateChannelNotification(
            this->split_->getChannel()->getName(), i);
    });
    menu->addAction(action);

    menu->addSeparator();
    menu->addAction("Reload channel emotes", this,
                    SLOT(menuReloadChannelEmotes()));
    menu->addAction("Reconnect", this, SLOT(menuManualReconnect()));
    //    menu->addAction("Clear messages", this->split_, &Split::doClearChat);
    //    menu->addSeparator();
    //    menu->addAction("Show changelog", this, SLOT(menuShowChangelog()));

    return menu;
}

std::unique_ptr<QMenu> SplitHeader::createChatModeMenu()
{
    auto menu = std::make_unique<QMenu>();

    auto setSub = new QAction("Subscriber only", this);
    auto setEmote = new QAction("Emote only", this);
    auto setSlow = new QAction("Slow", this);
    auto setR9k = new QAction("R9K", this);

    setSub->setCheckable(true);
    setEmote->setCheckable(true);
    setSlow->setCheckable(true);
    setR9k->setCheckable(true);

    menu->addAction(setEmote);
    menu->addAction(setSub);
    menu->addAction(setSlow);
    menu->addAction(setR9k);

    this->managedConnections_.push_back(this->modeUpdateRequested_.connect(  //
        [this, setSub, setEmote, setSlow, setR9k]() {
            auto twitchChannel =
                dynamic_cast<TwitchChannel *>(this->split_->getChannel().get());
            if (twitchChannel == nullptr) {
                this->modeButton_->hide();
                return;
            }

            auto roomModes = twitchChannel->accessRoomModes();

            setR9k->setChecked(roomModes->r9k);
            setSlow->setChecked(roomModes->slowMode);
            setEmote->setChecked(roomModes->emoteOnly);
            setSub->setChecked(roomModes->submode);
        }));

    auto toggle = [this](const QString &_command, QAction *action) mutable {
        QString command = _command;

        if (!action->isChecked()) {
            command += "off";
        };
        action->setChecked(!action->isChecked());

        qDebug() << command;
        this->split_->getChannel().get()->sendMessage(command);
    };

    QObject::connect(
        setSub, &QAction::triggered, this,
        [setSub, toggle]() mutable { toggle("/subscribers", setSub); });

    QObject::connect(
        setEmote, &QAction::triggered, this,
        [setEmote, toggle]() mutable { toggle("/emoteonly", setEmote); });

    QObject::connect(setSlow, &QAction::triggered, this, [setSlow, this]() {
        if (!setSlow->isChecked()) {
            this->split_->getChannel().get()->sendMessage("/slowoff");
            setSlow->setChecked(false);
            return;
        };
        bool ok;
        int slowSec = QInputDialog::getInt(this, "", "Seconds:", 10, 0, 500, 1,
                                           &ok, Qt::FramelessWindowHint);
        if (ok) {
            this->split_->getChannel().get()->sendMessage(
                QString("/slow %1").arg(slowSec));
        } else {
            setSlow->setChecked(false);
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

void SplitHeader::setupModeLabel(RippleEffectLabel &label)
{
    this->managedConnections_.push_back(
        this->modeUpdateRequested_.connect([this, &label] {
            auto twitchChannel =
                dynamic_cast<TwitchChannel *>(this->split_->getChannel().get());

            // return if the channel is not a twitch channel
            if (twitchChannel == nullptr) {
                label.hide();
                return;
            }

            // set lable enabled
            label.setEnable(twitchChannel->hasModRights());

            // set the label text
            QString text;

            {
                auto roomModes = twitchChannel->accessRoomModes();

                if (roomModes->r9k) text += "r9k, ";
                if (roomModes->slowMode)
                    text += QString("slow(%1), ")
                                .arg(QString::number(roomModes->slowMode));
                if (roomModes->emoteOnly) text += "emote, ";
                if (roomModes->submode) text += "sub, ";
            }

            if (text.length() > 2) {
                text = text.mid(0, text.size() - 2);
            }

            if (text.isEmpty()) {
                if (twitchChannel->hasModRights()) {
                    label.getLabel().setText("none");
                    label.show();
                } else {
                    label.hide();
                }
            } else {
                static QRegularExpression commaReplacement("^.+?, .+?,( ).+$");
                QRegularExpressionMatch match = commaReplacement.match(text);
                if (match.hasMatch()) {
                    text = text.mid(0, match.capturedStart(1)) + '\n' +
                           text.mid(match.capturedEnd(1));
                }

                label.getLabel().setText(text);
                label.show();
            }
        }));
}

void SplitHeader::initializeChannelSignals()
{
    // Disconnect any previous signal first
    this->onlineStatusChangedConnection_.disconnect();

    auto channel = this->split_->getChannel();
    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel) {
        this->managedConnections_.emplace_back(
            twitchChannel->liveStatusChanged.connect([this]() {
                this->updateChannelText();  //
            }));
    }
}

void SplitHeader::scaleChangedEvent(float scale)
{
    int w = int(28 * scale);

    this->setFixedHeight(w);
    this->dropdownButton_->setFixedWidth(w);
    this->moderationButton_->setFixedWidth(w);
    //    this->titleLabel->setFont(
    //        FontManager::getInstance().getFont(FontStyle::Medium, scale));
}

void SplitHeader::updateChannelText()
{
    auto indirectChannel = this->split_->getIndirectChannel();
    auto channel = this->split_->getChannel();

    QString title = channel->getName();

    if (indirectChannel.getType() == Channel::Type::TwitchWatching) {
        title = "watching: " + (title.isEmpty() ? "none" : title);
    }

    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel != nullptr) {
        const auto streamStatus = twitchChannel->accessStreamStatus();

        if (streamStatus->live) {
            this->isLive_ = true;
            this->tooltip_ = "<style>.center    { text-align: center; }</style>"
                             "<p class = \"center\">" +
                             streamStatus->title + "<br><br>" +
                             streamStatus->game + "<br>" +
                             (streamStatus->rerun ? "Vod-casting" : "Live") +
                             " for " + streamStatus->uptime + " with " +
                             QString::number(streamStatus->viewerCount) +
                             " viewers"
                             "</p>";
            if (streamStatus->rerun) {
                title += " (rerun)";
            } else if (streamStatus->streamType.isEmpty()) {
                title += " (" + streamStatus->streamType + ")";
            } else {
                title += " (live)";
            }
            if (getSettings()->showViewerCount) {
                title += " - " + QString::number(streamStatus->viewerCount) +
                         " viewers";
            }
            if (getSettings()->showTitle) {
                title += " - " + streamStatus->title;
            }
            if (getSettings()->showGame) {
                title += " - " + streamStatus->game;
            }
            if (getSettings()->showUptime) {
                title += "  - uptime: " + streamStatus->uptime;
            }
        } else {
            this->tooltip_ = QString();
        }
    }

    if (title.isEmpty()) {
        title = "<empty>";
    }

    this->isLive_ = false;
    this->titleLabel->setText(title);
}

void SplitHeader::updateModerationModeIcon()
{
    auto app = getApp();

    this->moderationButton_->setPixmap(
        this->split_->getModerationMode()
            ? app->resources->buttons.modModeEnabled
            : app->resources->buttons.modModeDisabled);

    bool modButtonVisible = false;
    ChannelPtr channel = this->split_->getChannel();

    auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel != nullptr && twitchChannel->hasModRights()) {
        modButtonVisible = true;
    }

    this->moderationButton_->setVisible(modButtonVisible);
}

void SplitHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), this->theme->splits.header.background);
    painter.setPen(this->theme->splits.header.border);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void SplitHeader::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->dragging_ = true;

        this->dragStart_ = event->pos();
    }

    this->doubleClicked_ = false;
}

void SplitHeader::mouseReleaseEvent(QMouseEvent *event)
{
    if (this->dragging_ && event->button() == Qt::LeftButton) {
        QPoint pos = event->globalPos();

        if (!showingHelpTooltip_) {
            this->showingHelpTooltip_ = true;

            QTimer::singleShot(400, this, [this, pos] {
                if (this->doubleClicked_) {
                    this->doubleClicked_ = false;
                    this->showingHelpTooltip_ = false;
                    return;
                }

                TooltipWidget *widget = new TooltipWidget();

                widget->setText("Double click or press <Ctrl+R> to change the "
                                "channel.\nClick and "
                                "drag to move the split.");
                widget->setAttribute(Qt::WA_DeleteOnClose);
                widget->move(pos);
                widget->show();
                widget->raise();

                QTimer::singleShot(3000, widget, [this, widget] {
                    widget->close();
                    this->showingHelpTooltip_ = false;
                });
            });
        }
    }

    this->dragging_ = false;
}

void SplitHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (this->dragging_) {
        if (std::abs(this->dragStart_.x() - event->pos().x()) >
                int(12 * this->getScale()) ||
            std::abs(this->dragStart_.y() - event->pos().y()) >
                int(12 * this->getScale())) {
            this->split_->drag();
            this->dragging_ = false;
        }
    }
}

void SplitHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->split_->doChangeChannel();
    }
    this->doubleClicked_ = true;
}

void SplitHeader::enterEvent(QEvent *event)
{
    if (!this->tooltip_.isEmpty()) {
        auto tooltipWidget = TooltipWidget::getInstance();
        tooltipWidget->moveTo(
            this, this->mapToGlobal(this->rect().bottomLeft()), false);
        tooltipWidget->setText(this->tooltip_);
        tooltipWidget->show();
        tooltipWidget->raise();
    }

    BaseWidget::enterEvent(event);
}

void SplitHeader::leaveEvent(QEvent *event)
{
    TooltipWidget::getInstance()->hide();

    BaseWidget::leaveEvent(event);
}

void SplitHeader::rightButtonClicked()
{
}

void SplitHeader::themeChangedEvent()
{
    QPalette palette;

    if (this->split_->hasFocus()) {
        palette.setColor(QPalette::Foreground,
                         this->theme->splits.header.focusedText);
    } else {
        palette.setColor(QPalette::Foreground, this->theme->splits.header.text);
    }

    if (this->theme->isLightTheme()) {
        this->dropdownButton_->setPixmap(getApp()->resources->buttons.menuDark);
    } else {
        this->dropdownButton_->setPixmap(
            getApp()->resources->buttons.menuLight);
    }

    this->titleLabel->setPalette(palette);
}

void SplitHeader::menuMoveSplit()
{
}

void SplitHeader::menuReloadChannelEmotes()
{
    auto channel = this->split_->getChannel();
    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel) {
        twitchChannel->refreshChannelEmotes();
    }
}

void SplitHeader::menuManualReconnect()
{
    auto app = getApp();

    // fourtf: connection
    app->twitch.server->connect();
}

void SplitHeader::menuShowChangelog()
{
}

}  // namespace chatterino
