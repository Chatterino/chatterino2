#include "widgets/splits/SplitHeader.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "util/LayoutHelper.hpp"
#include "widgets/Label.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QByteArray>
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

            if (modes->r9k) text += "r9k, ";
            if (modes->slowMode)
                text +=
                    QString("slow(%1), ").arg(QString::number(modes->slowMode));
            if (modes->emoteOnly) text += "emote, ";
            if (modes->submode) text += "sub, ";
        }

        if (text.length() > 2) {
            text = text.mid(0, text.size() - 2);
        }

        if (!text.isEmpty()) {
            static QRegularExpression commaReplacement("^(.+?, .+?,) (.+)$");

            auto match = commaReplacement.match(text);
            if (match.hasMatch())
                text = match.captured(1) + '\n' + match.captured(2);
        }

        if (text.isEmpty() && channel.hasModRights()) return "none";

        return text;
    }
    auto formatTooltip(const TwitchChannel::StreamStatus &s)
    {
        return QStringList{"<style>.center { text-align: center; }</style>",
                           "<p class=\"center\">",
                           s.title,
                           "<br><br>",
                           s.game,
                           "<br>",
                           s.rerun ? "Vod-casting" : "Live",
                           " for ",
                           s.uptime,
                           " with ",
                           QString::number(s.viewerCount),
                           " viewers",
                           "</p>"}
            .join("");
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
        if (settings.showUptime) title += " - " + s.uptime;
        if (settings.showViewerCount)
            title += " - " + QString::number(s.viewerCount);
        if (settings.showGame) title += " - " + s.game;
        if (settings.showTitle) title += " - " + s.title;

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
    getSettings()->showViewerCount.connect(_, this->managedConnections_);
    getSettings()->showTitle.connect(_, this->managedConnections_);
    getSettings()->showGame.connect(_, this->managedConnections_);
    getSettings()->showUptime.connect(_, this->managedConnections_);
}

void SplitHeader::initializeLayout()
{
    auto layout = makeLayout<QHBoxLayout>({
        // title
        this->titleLabel_ = makeWidget<Label>([](auto w) {
            w->setSizePolicy(QSizePolicy::MinimumExpanding,
                             QSizePolicy::Preferred);
            w->setCentered(true);
            w->setHasOffset(false);
        }),
        // mode
        this->modeButton_ = makeWidget<EffectLabel>([&](auto w) {
            w->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            w->hide();
            this->initializeModeSignals(*w);
            w->setMenu(this->createChatModeMenu());
        }),
        // moderator
        this->moderationButton_ = makeWidget<Button>([&](auto w) {
            QObject::connect(w, &Button::clicked, this, [this, w]() mutable {
                this->split_->setModerationMode(
                    !this->split_->getModerationMode());

                w->setDim(!this->split_->getModerationMode());
            });
        }),
        // dropdown
        this->dropdownButton_ = makeWidget<Button>(
            [&](auto w) { w->setMenu(this->createMainMenu()); }),
        // add split
        this->addButton_ = makeWidget<Button>([&](auto w) {
            w->setPixmap(getApp()->resources->buttons.addSplitDark);
            w->setEnableMargin(false);

            QObject::connect(w, &Button::clicked, this,
                             [this]() { this->split_->addSibling(); });
        }),
    });

    layout->setMargin(0);
    layout->setSpacing(0);
    this->setLayout(layout);
}

std::unique_ptr<QMenu> SplitHeader::createMainMenu()
{
    auto menu = std::make_unique<QMenu>();
    menu->addAction("Close channel", this->split_, &Split::deleteFromContainer,
                    QKeySequence("Ctrl+W"));
    menu->addAction("Change channel", this->split_, &Split::changeChannel,
                    QKeySequence("Ctrl+R"));
    menu->addSeparator();
    menu->addAction("Popup", this->split_, &Split::popup);
    menu->addAction("Viewer list", this->split_, &Split::showViewerList);
    menu->addAction("Search", this->split_, &Split::showSearch,
                    QKeySequence("Ctrl+F"));
    menu->addSeparator();
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
                    &Split::openBrowserPlayer);
#endif
    menu->addAction("Open streamlink", this->split_, &Split::openInStreamlink);

    auto action = new QAction(this);
    action->setText("Notify when live");
    action->setCheckable(true);

    QObject::connect(menu.get(), &QMenu::aboutToShow, this, [action, this]() {
        action->setChecked(getApp()->notifications->isChannelNotified(
            this->split_->getChannel()->getName(), Platform::Twitch));
    });
    action->connect(action, &QAction::triggered, this, [this]() {
        getApp()->notifications->updateChannelNotification(
            this->split_->getChannel()->getName(), Platform::Twitch);
    });

    menu->addAction(action);

    menu->addSeparator();
    menu->addAction("Reload channel emotes", this, SLOT(reloadChannelEmotes()));
    menu->addAction("Reload subscriber emotes", this, SLOT(reloadSubscriberEmotes()));
    menu->addAction("Reconnect", this, SLOT(reconnect()));
    menu->addAction("Clear messages", this->split_, &Split::clear);
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
        if (!setSlow->isChecked()) {
            this->split_->getChannel().get()->sendMessage("/slowoff");
            setSlow->setChecked(false);
            return;
        };
        auto ok = bool();
        auto seconds = QInputDialog::getInt(this, "", "Seconds:", 10, 0, 500, 1,
                                            &ok, Qt::FramelessWindowHint);
        if (ok) {
            this->split_->getChannel().get()->sendMessage(
                QString("/slow %1").arg(seconds));
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

void SplitHeader::initializeModeSignals(EffectLabel &label)
{
    this->modeUpdateRequested_.connect([this, &label] {
        if (auto twitchChannel = dynamic_cast<TwitchChannel *>(
                this->split_->getChannel().get()))  //
        {
            label.setEnable(twitchChannel->hasModRights());

            // set the label text
            auto text = formatRoomMode(*twitchChannel);

            if (!text.isEmpty()) {
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
    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get())) {
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

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get())) {
        const auto streamStatus = twitchChannel->accessStreamStatus();

        if (streamStatus->live) {
            this->isLive_ = true;
            this->tooltipText_ = formatTooltip(*streamStatus);
            title += formatTitle(*streamStatus, *getSettings());
        }
    }

    this->titleLabel_->setText(title.isEmpty() ? "<empty>" : title);
}

void SplitHeader::updateModerationModeIcon()
{
    this->moderationButton_->setPixmap(
        this->split_->getModerationMode()
            ? getApp()->resources->buttons.modModeEnabled
            : getApp()->resources->buttons.modModeDisabled);

    auto channel = this->split_->getChannel();
    auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel != nullptr && twitchChannel->hasModRights())
        this->moderationButton_->show();
    else
        this->moderationButton_->hide();
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
        auto pos = event->globalPos();

        if (!showingHelpTooltip_) {
            this->showingHelpTooltip_ = true;

            QTimer::singleShot(400, this, [this, pos] {
                if (this->doubleClicked_) {
                    this->doubleClicked_ = false;
                    this->showingHelpTooltip_ = false;
                    return;
                }

                auto tooltip = new TooltipWidget();

                tooltip->setText("Double click or press <Ctrl+R> to change the "
                                 "channel.\nClick and "
                                 "drag to move the split.");
                tooltip->setAttribute(Qt::WA_DeleteOnClose);
                tooltip->move(pos);
                tooltip->show();
                tooltip->raise();

                QTimer::singleShot(3000, tooltip, [this, tooltip] {
                    tooltip->close();
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
        if (distance(this->dragStart_, event->pos()) > 15 * this->getScale()) {
            this->split_->drag();
            this->dragging_ = false;
        }
    }
}

void SplitHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->split_->changeChannel();
    }
    this->doubleClicked_ = true;
}

void SplitHeader::enterEvent(QEvent *event)
{
    if (!this->tooltipText_.isEmpty()) {
        auto tooltip = TooltipWidget::getInstance();
        tooltip->moveTo(this, this->mapToGlobal(this->rect().bottomLeft()),
                        false);
        tooltip->setText(this->tooltipText_);
        tooltip->setWordWrap(false);
        tooltip->adjustSize();
        tooltip->show();
        tooltip->raise();
    }

    BaseWidget::enterEvent(event);
}

void SplitHeader::leaveEvent(QEvent *event)
{
    TooltipWidget::getInstance()->hide();

    BaseWidget::leaveEvent(event);
}

void SplitHeader::themeChangedEvent()
{
    auto palette = QPalette();

    if (this->split_->hasFocus()) {
        palette.setColor(QPalette::Foreground,
                         this->theme->splits.header.focusedText);
    } else {
        palette.setColor(QPalette::Foreground, this->theme->splits.header.text);
    }
    this->titleLabel_->setPalette(palette);

    // --
    if (this->theme->isLightTheme()) {
        this->dropdownButton_->setPixmap(getApp()->resources->buttons.menuDark);
        this->addButton_->setPixmap(getApp()->resources->buttons.addSplit);
    } else {
        this->dropdownButton_->setPixmap(
            getApp()->resources->buttons.menuLight);
        this->addButton_->setPixmap(getApp()->resources->buttons.addSplitDark);
    }
}

void SplitHeader::moveSplit()
{
}

void SplitHeader::reloadChannelEmotes()
{
    auto channel = this->split_->getChannel();

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
        twitchChannel->refreshChannelEmotes();
}

void SplitHeader::reloadSubscriberEmotes()
{
    getApp()->accounts->twitch.getCurrent()->loadEmotes();
}

void SplitHeader::reconnect()
{
    getApp()->twitch.server->connect();
}

}  // namespace chatterino
