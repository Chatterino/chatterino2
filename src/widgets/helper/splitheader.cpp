#include "widgets/helper/splitheader.hpp"

#include "application.hpp"
#include "providers/twitch/twitchchannel.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/thememanager.hpp"
#include "util/layoutcreator.hpp"
#include "util/urlfetch.hpp"
#include "widgets/label.hpp"
#include "widgets/split.hpp"
#include "widgets/splitcontainer.hpp"
#include "widgets/tooltipwidget.hpp"

#include <QByteArray>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

#ifdef USEWEBENGINE
#include "widgets/streamview.hpp"
#endif

using namespace chatterino::providers::twitch;

namespace chatterino {
namespace widgets {

SplitHeader::SplitHeader(Split *_split)
    : BaseWidget(_split)
    , split(_split)
{
    auto app = getApp();

    util::LayoutCreator<SplitHeader> layoutCreator(this);
    auto layout = layoutCreator.emplace<QHBoxLayout>().withoutMargin();
    {
        // dropdown label
        auto dropdown = layout.emplace<RippleEffectButton>(this).assign(&this->dropdownButton);
        dropdown->setMouseTracking(true);
        dropdown->setPixmap(*app->resources->splitHeaderContext->getPixmap());
        this->addDropdownItems(dropdown.getElement());
        QObject::connect(dropdown.getElement(), &RippleEffectButton::clicked, this, [this] {
            QTimer::singleShot(80, [&] {
                this->dropdownMenu.move(
                    this->dropdownButton->mapToGlobal(QPoint(0, this->dropdownButton->height())));
                this->dropdownMenu.show();
            });
        });

        // channel name label
        auto title = layout.emplace<Label>().assign(&this->titleLabel);
        title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        title->setCentered(true);
        title->setHasOffset(false);

        // mode button
        auto mode = layout.emplace<RippleEffectLabel>(this).assign(&this->modeButton);

        mode->hide();

        //        QObject::connect(mode.getElement(), &RippleEffectButton::clicked, this, [this]
        //        {
        //            //
        //        });

        // moderation mode
        auto moderator = layout.emplace<RippleEffectButton>(this).assign(&this->moderationButton);

        QObject::connect(moderator.getElement(), &RippleEffectButton::clicked, this, [this] {
            this->split->setModerationMode(!this->split->getModerationMode());
        });

        this->updateModerationModeIcon();
    }

    // ---- misc
    this->layout()->setMargin(0);
    this->scaleChangedEvent(this->getScale());

    this->updateChannelText();

    this->initializeChannelSignals();

    this->split->channelChanged.connect([this]() {
        this->initializeChannelSignals();  //
    });

    this->managedConnect(app->accounts->twitch.currentUserChanged,
                         [this] { this->updateModerationModeIcon(); });

    this->setMouseTracking(true);
}

SplitHeader::~SplitHeader()
{
    this->onlineStatusChangedConnection.disconnect();
}

void SplitHeader::addDropdownItems(RippleEffectButton *)
{
    // clang-format off
    this->dropdownMenu.addAction("Add new split", this->split, &Split::doAddSplit, QKeySequence(tr("Ctrl+T")));
    this->dropdownMenu.addAction("Close split", this->split, &Split::doCloseSplit, QKeySequence(tr("Ctrl+W")));
//    this->dropdownMenu.addAction("Move split", this, SLOT(menuMoveSplit()));
    this->dropdownMenu.addAction("Popup", this->split, &Split::doPopup);
    this->dropdownMenu.addAction("Open viewer list", this->split, &Split::doOpenViewerList);
    this->dropdownMenu.addAction("Search in messages", this->split, &Split::doSearch, QKeySequence(tr("Ctrl+F")));
    this->dropdownMenu.addSeparator();
#ifdef USEWEBENGINE
    this->dropdownMenu.addAction("Start watching", this, [this]{
        ChannelPtr _channel = this->split->getChannel();
        TwitchChannel *tc = dynamic_cast<TwitchChannel *>(_channel.get());

        if (tc != nullptr) {
            StreamView *view = new StreamView(_channel, "https://player.twitch.tv/?channel=" + tc->name);
            view->setAttribute(Qt::WA_DeleteOnClose, true);
            view->show();
        }
    });
#endif
    this->dropdownMenu.addAction("Change channel", this->split, &Split::doChangeChannel, QKeySequence(tr("Ctrl+R")));
    this->dropdownMenu.addAction("Clear chat", this->split, &Split::doClearChat);
    this->dropdownMenu.addAction("Open in web browser", this->split, &Split::doOpenChannel);
#ifndef USEWEBENGINE
    this->dropdownMenu.addAction("Open web player", this->split, &Split::doOpenPopupPlayer);
#endif
    this->dropdownMenu.addAction("Open in Streamlink", this->split, &Split::doOpenStreamlink);
    this->dropdownMenu.addSeparator();
    this->dropdownMenu.addAction("Reload channel emotes", this, SLOT(menuReloadChannelEmotes()));
    this->dropdownMenu.addAction("Manual reconnect", this, SLOT(menuManualReconnect()));
//    this->dropdownMenu.addSeparator();
//    this->dropdownMenu.addAction("Show changelog", this, SLOT(menuShowChangelog()));
    // clang-format on
}

void SplitHeader::initializeChannelSignals()
{
    // Disconnect any previous signal first
    this->onlineStatusChangedConnection.disconnect();

    auto channel = this->split->getChannel();
    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel) {
        this->managedConnections.emplace_back(twitchChannel->updateLiveInfo.connect([this]() {
            this->updateChannelText();  //
        }));
    }
}

void SplitHeader::scaleChangedEvent(float scale)
{
    int w = 28 * scale;

    this->setFixedHeight(w);
    this->dropdownButton->setFixedWidth(w);
    this->moderationButton->setFixedWidth(w);
    //    this->titleLabel->setFont(
    //        singletons::FontManager::getInstance().getFont(FontStyle::Medium, scale));
}

void SplitHeader::updateChannelText()
{
    auto indirectChannel = this->split->getIndirectChannel();
    auto channel = this->split->getChannel();

    QString title = channel->name;

    if (indirectChannel.getType() == Channel::TwitchWatching) {
        title = "watching: " + (title.isEmpty() ? "none" : title);
    }

    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel != nullptr) {
        const auto streamStatus = twitchChannel->getStreamStatus();

        if (streamStatus.live) {
            this->isLive = true;
            this->tooltip = "<style>.center    { text-align: center; }</style>"
                            "<p class = \"center\">" +
                            streamStatus.title + "<br><br>" + streamStatus.game + "<br>" +
                            (streamStatus.rerun ? "Vod-casting" : "Live") + " for " +
                            streamStatus.uptime + " with " +
                            QString::number(streamStatus.viewerCount) +
                            " viewers"
                            "</p>";
            if (streamStatus.rerun) {
                title += " (rerun)";
            } else if (streamStatus.streamType.isEmpty()) {
                title += " (" + streamStatus.streamType + ")";
            } else {
                title += " (live)";
            }
        } else {
            this->tooltip = QString();
        }
    }

    if (title.isEmpty()) {
        title = "<empty>";
    }

    this->isLive = false;
    this->titleLabel->setText(title);
}

void SplitHeader::updateModerationModeIcon()
{
    auto app = getApp();

    this->moderationButton->setPixmap(this->split->getModerationMode()
                                          ? *app->resources->moderationmode_enabled->getPixmap()
                                          : *app->resources->moderationmode_disabled->getPixmap());

    bool modButtonVisible = false;
    ChannelPtr channel = this->split->getChannel();

    TwitchChannel *tc = dynamic_cast<TwitchChannel *>(channel.get());

    if (tc != nullptr && tc->hasModRights()) {
        modButtonVisible = true;
    }

    this->moderationButton->setVisible(modButtonVisible);
}

void SplitHeader::updateModes()
{
    TwitchChannel *tc = dynamic_cast<TwitchChannel *>(this->split->getChannel().get());
    if (tc == nullptr) {
        this->modeButton->hide();
        return;
    }

    TwitchChannel::RoomModes roomModes = tc->getRoomModes();

    QString text;

    if (roomModes.r9k) {
        text += "r9k, ";
    }
    if (roomModes.slowMode) {
        text += QString("slow(%1), ").arg(QString::number(roomModes.slowMode));
    }
    if (roomModes.emoteOnly) {
        text += "emote, ";
    }
    if (roomModes.submode) {
        text += "sub, ";
    }

    if (text.length() > 2) {
        text = text.mid(0, text.size() - 2);
    }

    if (text.isEmpty()) {
        this->modeButton->hide();
    } else {
        static QRegularExpression commaReplacement("^.+?, .+?,( ).+$");
        QRegularExpressionMatch match = commaReplacement.match(text);
        if (match.hasMatch()) {
            text = text.mid(0, match.capturedStart(1)) + '\n' + text.mid(match.capturedEnd(1));
        }

        this->modeButton->getLabel().setText(text);
        this->modeButton->show();
    }
}

void SplitHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), this->themeManager->splits.header.background);
    painter.setPen(this->themeManager->splits.header.border);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void SplitHeader::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->dragging = true;

        this->dragStart = event->pos();
    }

    this->doubleClicked = false;
}

void SplitHeader::mouseReleaseEvent(QMouseEvent *event)
{
    if (this->dragging && event->button() == Qt::LeftButton) {
        QPoint pos = event->globalPos();

        if (!showingHelpTooltip) {
            this->showingHelpTooltip = true;

            QTimer::singleShot(400, this, [this, pos] {
                if (this->doubleClicked) {
                    this->doubleClicked = false;
                    this->showingHelpTooltip = false;
                    return;
                }

                TooltipWidget *widget = new TooltipWidget();

                widget->setText("Double click or press <Ctrl+R> to change the channel.\nClick and "
                                "drag to move the split.");
                widget->setAttribute(Qt::WA_DeleteOnClose);
                widget->move(pos);
                widget->show();
                widget->raise();

                QTimer::singleShot(3000, widget, [this, widget] {
                    widget->close();
                    this->showingHelpTooltip = false;
                });
            });
        }
    }

    this->dragging = false;
}

void SplitHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (this->dragging) {
        if (std::abs(this->dragStart.x() - event->pos().x()) > int(12 * this->getScale()) ||
            std::abs(this->dragStart.y() - event->pos().y()) > int(12 * this->getScale())) {
            this->split->drag();
            this->dragging = false;
        }
    }
}

void SplitHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->split->doChangeChannel();
    }
    this->doubleClicked = true;
}

void SplitHeader::enterEvent(QEvent *event)
{
    if (!this->tooltip.isEmpty()) {
        auto tooltipWidget = TooltipWidget::getInstance();
        tooltipWidget->moveTo(this, this->mapToGlobal(this->rect().bottomLeft()), false);
        tooltipWidget->setText(this->tooltip);
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

void SplitHeader::themeRefreshEvent()
{
    QPalette palette;
    palette.setColor(QPalette::Foreground, this->themeManager->splits.header.text);

    //    this->dropdownButton->setPalette(palette);
    this->titleLabel->setPalette(palette);
    //    this->moderationLabel->setPalette(palette);
}

void SplitHeader::menuMoveSplit()
{
}

void SplitHeader::menuReloadChannelEmotes()
{
    auto channel = this->split->getChannel();
    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel) {
        twitchChannel->reloadChannelEmotes();
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

}  // namespace widgets
}  // namespace chatterino
