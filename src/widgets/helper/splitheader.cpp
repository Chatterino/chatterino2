#include "widgets/helper/splitheader.hpp"
#include "colorscheme.hpp"
#include "twitch/twitchchannel.hpp"
#include "util/urlfetch.hpp"
#include "widgets/split.hpp"
#include "widgets/splitcontainer.hpp"
#include "widgets/tooltipwidget.hpp"

#include <QByteArray>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

namespace chatterino {
namespace widgets {

SplitHeader::SplitHeader(Split *_chatWidget)
    : BaseWidget(_chatWidget)
    , chatWidget(_chatWidget)
    , leftLabel(this)
    , leftMenu(this)
    , rightLabel(this)
    , rightMenu(this)
{
    this->setMouseTracking(true);
    this->leftLabel.setMouseTracking(true);
    this->channelNameLabel.setMouseTracking(true);
    this->rightLabel.setMouseTracking(true);

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

    connect(&this->leftLabel, &RippleEffectLabel::clicked, this, &SplitHeader::leftButtonClicked);

    this->leftMenu.addAction("Add new split", this->chatWidget, &Split::doAddSplit,
                             QKeySequence(tr("Ctrl+T")));
    this->leftMenu.addAction("Close split", this->chatWidget, &Split::doCloseSplit,
                             QKeySequence(tr("Ctrl+W")));
    this->leftMenu.addAction("Move split", this, SLOT(menuMoveSplit()));
    this->leftMenu.addAction("Popup", this->chatWidget, &Split::doPopup);
    this->leftMenu.addAction("Open viewer list", this->chatWidget, &Split::doOpenViewerList);
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Change channel", this->chatWidget, &Split::doChangeChannel,
                             QKeySequence(tr("Ctrl+R")));
    this->leftMenu.addAction("Clear chat", this->chatWidget, &Split::doClearChat);
    this->leftMenu.addAction("Open channel", this->chatWidget, &Split::doOpenChannel);
    this->leftMenu.addAction("Open popup player", this->chatWidget, &Split::doOpenPopupPlayer);
    this->leftMenu.addAction("Open in Streamlink", this->chatWidget, &Split::doOpenStreamlink);
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Reload channel emotes", this, SLOT(menuReloadChannelEmotes()));
    this->leftMenu.addAction("Manual reconnect", this, SLOT(menuManualReconnect()));
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Show changelog", this, SLOT(menuShowChangelog()));

    // middle
    this->channelNameLabel.setAlignment(Qt::AlignCenter);

    connect(&this->channelNameLabel, &SignalLabel::mouseDoubleClick, this,
            &SplitHeader::mouseDoubleClickEvent);

    // right
    this->rightLabel.setMinimumWidth(this->height());
    this->rightLabel.getLabel().setTextFormat(Qt::RichText);
    this->rightLabel.getLabel().setText("ayy");

    this->initializeChannelSignals();

    this->chatWidget->channelChanged.connect([this]() {
        this->initializeChannelSignals();  //
    });
}

void SplitHeader::initializeChannelSignals()
{
    // Disconnect any previous signal first
    this->onlineStatusChangedConnection.disconnect();

    auto channel = this->chatWidget->getChannel();
    twitch::TwitchChannel *twitchChannel = dynamic_cast<twitch::TwitchChannel *>(channel.get());

    if (twitchChannel) {
        twitchChannel->onlineStatusChanged.connect([this]() {
            this->updateChannelText();  //
        });
    }
}

void SplitHeader::resizeEvent(QResizeEvent *event)
{
    this->setFixedHeight(static_cast<float>(28 * getDpiMultiplier()));
}

void SplitHeader::updateChannelText()
{
    const std::string channelName = this->chatWidget->channelName;
    if (channelName.empty()) {
        this->channelNameLabel.setText("<no channel>");
    } else {
        auto channel = this->chatWidget->getChannel();

        twitch::TwitchChannel *twitchChannel = dynamic_cast<twitch::TwitchChannel *>(channel.get());

        if (twitchChannel != nullptr && twitchChannel->isLive) {
            this->isLive = true;
            this->tooltip = "<style>.center    { text-align: center; }</style>"
                            "<p class = \"center\">" +
                            twitchChannel->streamStatus + "<br><br>" + twitchChannel->streamGame +
                            "<br>"
                            "Live for " +
                            twitchChannel->streamUptime + " with " +
                            twitchChannel->streamViewerCount +
                            " viewers"
                            "</p>";
            this->channelNameLabel.setText(QString::fromStdString(channelName) + " (live)");
        } else {
            this->isLive = false;
            this->channelNameLabel.setText(QString::fromStdString(channelName));
            this->tooltip = "";
        }
    }
}

void SplitHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), this->colorScheme.ChatHeaderBackground);
    painter.setPen(this->colorScheme.ChatHeaderBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void SplitHeader::mousePressEvent(QMouseEvent *event)
{
    this->dragging = true;

    this->dragStart = event->pos();
}

void SplitHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->dragging && this->isLive) {
        auto tooltipWidget = TooltipWidget::getInstance();
        tooltipWidget->moveTo(event->globalPos());
        tooltipWidget->setText(tooltip);
        tooltipWidget->show();
    }

    if (this->dragging) {
        if (std::abs(this->dragStart.x() - event->pos().x()) > 12 ||
            std::abs(this->dragStart.y() - event->pos().y()) > 12) {
            auto page = static_cast<SplitContainer *>(this->chatWidget->parentWidget());

            if (page != nullptr) {
                SplitContainer::isDraggingSplit = true;
                SplitContainer::draggingSplit = this->chatWidget;

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

                SplitContainer::isDraggingSplit = false;
                this->dragging = false;
            }
        }
    }
}

void SplitHeader::leaveEvent(QEvent *event)
{
    TooltipWidget::getInstance()->hide();
    BaseWidget::leaveEvent(event);
}

void SplitHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->chatWidget->doChangeChannel();
    }
}

void SplitHeader::leftButtonClicked()
{
    QTimer::singleShot(80, [&] {
        this->leftMenu.move(this->leftLabel.mapToGlobal(QPoint(0, this->leftLabel.height())));
        this->leftMenu.show();
    });
}

void SplitHeader::rightButtonClicked()
{
}

void SplitHeader::refreshTheme()
{
    QPalette palette;
    palette.setColor(QPalette::Foreground, this->colorScheme.Text);

    this->leftLabel.setPalette(palette);
    this->channelNameLabel.setPalette(palette);
    this->rightLabel.setPalette(palette);
}

void SplitHeader::menuMoveSplit()
{
}

void SplitHeader::menuReloadChannelEmotes()
{
}

void SplitHeader::menuManualReconnect()
{
}

void SplitHeader::menuShowChangelog()
{
}

}  // namespace widgets
}  // namespace chatterino
