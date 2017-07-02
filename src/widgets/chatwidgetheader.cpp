#include "widgets/chatwidgetheader.hpp"
#include "colorscheme.hpp"
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

    connect(&this->leftLabel, &ChatWidgetHeaderButton::clicked, this,
            &ChatWidgetHeader::leftButtonClicked);

    this->leftMenu.addAction("Add new split", this->chatWidget, &ChatWidget::doAddSplit,
                             QKeySequence(tr("Ctrl+T")));
    this->leftMenu.addAction("Close split", this->chatWidget, &ChatWidget::doCloseSplit,
                             QKeySequence(tr("Ctrl+W")));
    this->leftMenu.addAction("Move split", this, SLOT(menuMoveSplit()));
    this->leftMenu.addAction("Popup", this->chatWidget, &ChatWidget::doPopup);
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Change channel", this->chatWidget, &ChatWidget::doChangeChannel,
                             QKeySequence(tr("Ctrl+R")));
    this->leftMenu.addAction("Clear chat", this->chatWidget, &ChatWidget::doClearChat);
    this->leftMenu.addAction("Open channel", this->chatWidget, &ChatWidget::doOpenChannel);
    this->leftMenu.addAction("Open popup player", this->chatWidget, &ChatWidget::doOpenPopupPlayer);
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
}

void ChatWidgetHeader::updateChannelText()
{
    const QString &c = this->chatWidget->getChannelName();

    this->channelNameLabel.setText(c.isEmpty() ? "<no channel>" : c);
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
        this->chatWidget->showChangeChannelPopup();
    }
}

void ChatWidgetHeader::leftButtonClicked()
{
    this->leftMenu.move(this->leftLabel.mapToGlobal(QPoint(0, this->leftLabel.height())));
    this->leftMenu.show();
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

}  // namespace widgets
}  // namespace chatterino
