#include "widgets/chatwidgetheader.h"
#include "colorscheme.h"
#include "widgets/chatwidget.h"
#include "widgets/notebookpage.h"

#include <QByteArray>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

namespace chatterino {
namespace widgets {

ChatWidgetHeader::ChatWidgetHeader(ChatWidget *parent)
    : QWidget()
    , chatWidget(parent)
    , dragStart()
    , dragging(false)
    , leftLabel()
    , middleLabel()
    , rightLabel()
    , leftMenu(this)
    , rightMenu(this)
{
    setFixedHeight(32);

    updateColors();
    updateChannelText();

    setLayout(&this->hbox);
    this->hbox.setMargin(0);
    this->hbox.addWidget(&this->leftLabel);
    this->hbox.addWidget(&this->middleLabel, 1);
    this->hbox.addWidget(&this->rightLabel);

    // left
    this->leftLabel.getLabel().setTextFormat(Qt::RichText);
    this->leftLabel.getLabel().setText(
        "<img src=':/images/tool_moreCollapser_off16.png' />");

    QObject::connect(&this->leftLabel, &ChatWidgetHeaderButton::clicked, this,
                     &ChatWidgetHeader::leftButtonClicked);

    this->leftMenu.addAction("Add new split", this, SLOT(menuAddSplit()),
                             QKeySequence(tr("Ctrl+T")));
    this->leftMenu.addAction("Close split", this, SLOT(menuCloseSplit()),
                             QKeySequence(tr("Ctrl+W")));
    this->leftMenu.addAction("Move split", this, SLOT(menuMoveSplit()));
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Change channel", this, SLOT(menuChangeChannel()),
                             QKeySequence(tr("Ctrl+R")));
    this->leftMenu.addAction("Clear chat", this, SLOT(menuClearChat()));
    this->leftMenu.addAction("Open channel", this, SLOT(menuOpenChannel()));
    this->leftMenu.addAction("Open pop-out player", this,
                             SLOT(menuPopupPlayer()));
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Reload channel emotes", this,
                             SLOT(menuReloadChannelEmotes()));
    this->leftMenu.addAction("Manual reconnect", this,
                             SLOT(menuManualReconnect()));
    this->leftMenu.addSeparator();
    this->leftMenu.addAction("Show changelog", this, SLOT(menuShowChangelog()));

    // middle
    this->middleLabel.setAlignment(Qt::AlignCenter);

    connect(&this->middleLabel, &SignalLabel::mouseDoubleClick, this,
            &ChatWidgetHeader::mouseDoubleClickEvent);
    //    connect(&this->middleLabel, &SignalLabel::mouseDown, this,
    //            &ChatWidgetHeader::mouseDoubleClickEvent);

    // right
    this->rightLabel.setMinimumWidth(height());
    this->rightLabel.getLabel().setTextFormat(Qt::RichText);
    this->rightLabel.getLabel().setText("ayy");
}

void
ChatWidgetHeader::updateColors()
{
    QPalette palette;
    palette.setColor(QPalette::Foreground, ColorScheme::getInstance().Text);

    this->leftLabel.setPalette(palette);
    this->middleLabel.setPalette(palette);
    this->rightLabel.setPalette(palette);
}

void
ChatWidgetHeader::updateChannelText()
{
    const QString &c = this->chatWidget->getChannelName();

    this->middleLabel.setText(c.isEmpty() ? "<no channel>" : c);
}

void
ChatWidgetHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::getInstance().ChatHeaderBackground);
    painter.setPen(ColorScheme::getInstance().ChatHeaderBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void
ChatWidgetHeader::mousePressEvent(QMouseEvent *event)
{
    this->dragging = true;

    this->dragStart = event->pos();
}

void
ChatWidgetHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (this->dragging) {
        if (std::abs(this->dragStart.x() - event->pos().x()) > 12 ||
            std::abs(this->dragStart.y() - event->pos().y()) > 12) {
            auto chatWidget = this->chatWidget;
            auto page = static_cast<NotebookPage *>(chatWidget->parentWidget());

            if (page != NULL) {
                NotebookPage::isDraggingSplit = true;
                NotebookPage::draggingSplit = chatWidget;

                auto originalLocation = page->removeFromLayout(chatWidget);

                // page->update();

                QDrag *drag = new QDrag(chatWidget);
                QMimeData *mimeData = new QMimeData;

                mimeData->setData("chatterino/split", "xD");

                drag->setMimeData(mimeData);

                Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

                if (dropAction == Qt::IgnoreAction) {
                    page->addToLayout(chatWidget, originalLocation);
                }

                NotebookPage::isDraggingSplit = false;
            }
        }
    }
}

void
ChatWidgetHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->chatWidget->showChangeChannelPopup();
    }
}

void
ChatWidgetHeader::leftButtonClicked()
{
    this->leftMenu.move(
        this->leftLabel.mapToGlobal(QPoint(0, this->leftLabel.height())));
    this->leftMenu.show();
}

void
ChatWidgetHeader::rightButtonClicked()
{
}

void
ChatWidgetHeader::menuAddSplit()
{
}
void
ChatWidgetHeader::menuCloseSplit()
{
}
void
ChatWidgetHeader::menuMoveSplit()
{
}
void
ChatWidgetHeader::menuChangeChannel()
{
    this->chatWidget->showChangeChannelPopup();
}
void
ChatWidgetHeader::menuClearChat()
{
}
void
ChatWidgetHeader::menuOpenChannel()
{
}
void
ChatWidgetHeader::menuPopupPlayer()
{
}
void
ChatWidgetHeader::menuReloadChannelEmotes()
{
}
void
ChatWidgetHeader::menuManualReconnect()
{
}
void
ChatWidgetHeader::menuShowChangelog()
{
}
}
}
