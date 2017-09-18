#include "widgets/chatwidgetheader.hpp"
#include "colorscheme.hpp"
#include "twitch/twitchchannel.hpp"
#include "util/urlfetch.hpp"
#include "widgets/chatwidget.hpp"
#include "widgets/notebookpage.hpp"

#include <QByteArray>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetHeader::ChatWidgetHeader(ChatWidget *_chatWidget)
    : BaseWidget(_chatWidget)
    , chatWidget(_chatWidget)
    , leftLabel(this)
    , leftMenu(this)
    , rightLabel(this)
    , rightMenu(this)
{
    this->setFixedHeight(32);

    this->refreshTheme();

    this->updateChannelText();

    this->setLayout(&this->hbox);
    this->hbox.setMargin(0);
    this->hbox.addWidget(&this->leftLabel);
    this->hbox.addWidget(&this->channelNameLabel, 1);
    this->hbox.addWidget(&this->rightLabel);

    // left
    this->leftLabel.getLabel().setTextFormat(Qt::RichText);
    this->leftLabel.getLabel().setText("<img src=':/images/tool_moreCollapser_off16.png' />");

    connect(&this->leftLabel, &RippleEffectLabel::clicked, this,
            &ChatWidgetHeader::leftButtonClicked);

    this->leftMenu.addAction("Add new split", this->chatWidget, &ChatWidget::doAddSplit,
                             QKeySequence(tr("Ctrl+T")));
    this->leftMenu.addAction("Close split", this->chatWidget, &ChatWidget::doCloseSplit,
                             QKeySequence(tr("Ctrl+W")));
    this->leftMenu.addAction("Move split", this, SLOT(menuMoveSplit()));
    this->leftMenu.addAction("Popup", this->chatWidget, &ChatWidget::doPopup);
    this->leftMenu.addAction("Open viewer list", this->chatWidget, &ChatWidget::doOpenViewerList);
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Change channel", this->chatWidget, &ChatWidget::doChangeChannel,
                             QKeySequence(tr("Ctrl+R")));
    this->leftMenu.addAction("Clear chat", this->chatWidget, &ChatWidget::doClearChat);
    this->leftMenu.addAction("Open channel", this->chatWidget, &ChatWidget::doOpenChannel);
    this->leftMenu.addAction("Open popup player", this->chatWidget, &ChatWidget::doOpenPopupPlayer);
    this->leftMenu.addAction("Open in Streamlink", this->chatWidget, &ChatWidget::doOpenStreamlink);
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Reload channel emotes", this, SLOT(menuReloadChannelEmotes()));
    this->leftMenu.addAction("Manual reconnect", this, SLOT(menuManualReconnect()));
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Show changelog", this, SLOT(menuShowChangelog()));

    // middle
    this->channelNameLabel.setAlignment(Qt::AlignCenter);

    connect(&this->channelNameLabel, &SignalLabel::mouseDoubleClick, this,
            &ChatWidgetHeader::mouseDoubleClickEvent);

    // right
    this->rightLabel.setMinimumWidth(this->height());
    this->rightLabel.getLabel().setTextFormat(Qt::RichText);
    this->rightLabel.getLabel().setText("ayy");

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ChatWidgetHeader::checkLive);
    timer->start(60000);
}

void ChatWidgetHeader::updateChannelText()
{
    const std::string channelName = this->chatWidget->channelName;
    if (channelName.empty()) {
        this->channelNameLabel.setText("<no channel>");
    } else {
        auto channel = this->chatWidget->getChannel();

        twitch::TwitchChannel *twitchChannel = dynamic_cast<twitch::TwitchChannel *>(channel.get());
        if (twitchChannel->isLive) {
            this->channelNameLabel.setText(QString::fromStdString(channelName) + " (live)");
            if (twitchChannel != nullptr) {
                this->setToolTip("<style>.center    { text-align: center; }</style>"
                                 "<p class = \"center\">" +
                                 twitchChannel->streamStatus + "<br><br>" +
                                 twitchChannel->streamGame + "<br>"
                                                             "Live for " +
                                 twitchChannel->streamUptime + " with " +
                                 twitchChannel->streamViewerCount + " viewers"
                                                                    "</p>");
            }
        } else {
            this->channelNameLabel.setText(QString::fromStdString(channelName));
            this->setToolTip("");
        }
    }
}

void ChatWidgetHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), this->colorScheme.ChatHeaderBackground);
    painter.setPen(this->colorScheme.ChatHeaderBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void ChatWidgetHeader::mousePressEvent(QMouseEvent *event)
{
    this->dragging = true;

    this->dragStart = event->pos();
}

void ChatWidgetHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (this->dragging) {
        if (std::abs(this->dragStart.x() - event->pos().x()) > 12 ||
            std::abs(this->dragStart.y() - event->pos().y()) > 12) {
            auto page = static_cast<NotebookPage *>(this->chatWidget->parentWidget());

            if (page != nullptr) {
                NotebookPage::isDraggingSplit = true;
                NotebookPage::draggingSplit = this->chatWidget;

                auto originalLocation = page->removeFromLayout(this->chatWidget);

                // page->update();

                QDrag *drag = new QDrag(this->chatWidget);
                QMimeData *mimeData = new QMimeData;

                mimeData->setData("chatterino/split", "xD");

                drag->setMimeData(mimeData);

                Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

                if (dropAction == Qt::IgnoreAction) {
                    page->addToLayout(this->chatWidget, originalLocation);
                }

                NotebookPage::isDraggingSplit = false;
            }
        }
    }
}

void ChatWidgetHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->chatWidget->doChangeChannel();
    }
}

void ChatWidgetHeader::leftButtonClicked()
{
    QTimer::singleShot(80, [&] {
        this->leftMenu.move(this->leftLabel.mapToGlobal(QPoint(0, this->leftLabel.height())));
        this->leftMenu.show();
    });
}

void ChatWidgetHeader::rightButtonClicked()
{
}

void ChatWidgetHeader::refreshTheme()
{
    QPalette palette;
    palette.setColor(QPalette::Foreground, this->colorScheme.Text);

    this->leftLabel.setPalette(palette);
    this->channelNameLabel.setPalette(palette);
    this->rightLabel.setPalette(palette);
}

void ChatWidgetHeader::menuMoveSplit()
{
}

void ChatWidgetHeader::menuReloadChannelEmotes()
{
}

void ChatWidgetHeader::menuManualReconnect()
{
}

void ChatWidgetHeader::menuShowChangelog()
{
}

// TODO: this needs to be moved out of here
void ChatWidgetHeader::checkLive()
{
    twitch::TwitchChannel *channel =
        dynamic_cast<twitch::TwitchChannel *>(this->chatWidget->getChannel().get());

    if (channel == nullptr) {
        return;
    }

    auto id = QString::fromStdString(channel->roomID);

    util::twitch::get("https://api.twitch.tv/kraken/streams/" + id, [=](QJsonObject obj) {
        if (obj.value("stream").isNull()) {
            channel->isLive = false;
            this->updateChannelText();
        } else {
            channel->isLive = true;
            auto stream = obj.value("stream").toObject();
            channel->streamViewerCount = QString::number(stream.value("viewers").toDouble());
            channel->streamGame = stream.value("game").toString();
            channel->streamStatus = stream.value("channel").toObject().value("status").toString();
            QDateTime since =
                QDateTime::fromString(stream.value("created_at").toString(), Qt::ISODate);
            auto diff = since.secsTo(QDateTime::currentDateTime());
            channel->streamUptime =
                QString::number(diff / 3600) + "h " + QString::number(diff % 3600 / 60) + "m";
            this->updateChannelText();
        }
    });
}

}  // namespace widgets
}  // namespace chatterino
