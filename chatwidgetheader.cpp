#include "chatwidgetheader.h"
#include "chatwidget.h"
#include "colorscheme.h"
#include "notebookpage.h"

#include <QByteArray>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

ChatWidgetHeader::ChatWidgetHeader()
    : QWidget()
    , m_dragStart()
    , m_dragging(false)
    , leftLabel()
    , middleLabel()
    , rightLabel()
    , leftMenu(this)
    , rightMenu(this)
{
    setFixedHeight(32);

    updateColors();

    setLayout(&hbox);
    hbox.setMargin(0);
    hbox.addWidget(&leftLabel);
    hbox.addWidget(&middleLabel, 1);
    hbox.addWidget(&rightLabel);

    // left
    leftLabel.label().setTextFormat(Qt::RichText);
    leftLabel.label().setText(
        "<img src=':/images/tool_moreCollapser_off16.png' />");

    QObject::connect(&leftLabel, ChatWidgetHeaderButton::clicked, this,
                     leftButtonClicked);

    leftMenu.addAction("Add new split", this, ChatWidgetHeader::menuAddSplit,
                       QKeySequence(tr("Ctrl+T")));
    leftMenu.addAction("Close split", this, ChatWidgetHeader::menuCloseSplit,
                       QKeySequence(tr("Ctrl+W")));
    leftMenu.addAction("Move split", this, ChatWidgetHeader::menuMoveSplit);
    leftMenu.addSeparator();
    leftMenu.addAction("Change channel", this,
                       ChatWidgetHeader::menuChangeChannel,
                       QKeySequence(tr("Ctrl+R")));
    leftMenu.addAction("Clear chat", this, ChatWidgetHeader::menuClearChat);
    leftMenu.addAction("Open channel", this, ChatWidgetHeader::menuOpenChannel);
    leftMenu.addAction("Open pop-out player", this,
                       ChatWidgetHeader::menuPopupPlayer);
    leftMenu.addSeparator();
    leftMenu.addAction("Reload channel emotes", this,
                       ChatWidgetHeader::menuReloadChannelEmotes);
    leftMenu.addAction("Manual reconnect", this,
                       ChatWidgetHeader::menuManualReconnect);
    leftMenu.addSeparator();
    leftMenu.addAction("Show changelog", this,
                       ChatWidgetHeader::menuShowChangelog);

    // middle
    middleLabel.setAlignment(Qt::AlignCenter);
    middleLabel.setText("textString");

    // right
    rightLabel.setMinimumWidth(height());
    rightLabel.label().setTextFormat(Qt::RichText);
    rightLabel.label().setText("ayy");
}

void
ChatWidgetHeader::updateColors()
{
    QPalette palette;
    palette.setColor(QPalette::Foreground, ColorScheme::instance().Text);

    leftLabel.setPalette(palette);
    middleLabel.setPalette(palette);
    rightLabel.setPalette(palette);
}

void
ChatWidgetHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::instance().ChatHeaderBackground);
    painter.setPen(ColorScheme::instance().ChatHeaderBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void
ChatWidgetHeader::mousePressEvent(QMouseEvent *event)
{
    m_dragging = true;

    m_dragStart = event->pos();
}

void
ChatWidgetHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        if (std::abs(m_dragStart.x() - event->pos().x()) > 12 ||
            std::abs(m_dragStart.y() - event->pos().y()) > 12) {
            auto chatWidget = getChatWidget();
            auto page = static_cast<NotebookPage *>(chatWidget->parentWidget());

            if (page != NULL) {
                NotebookPage::isDraggingSplit = true;
                NotebookPage::draggingSplit = chatWidget;

                auto originalLocation = page->removeFromLayout(chatWidget);

                // page->repaint();

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

ChatWidget *
ChatWidgetHeader::getChatWidget()
{
    return static_cast<ChatWidget *>(parentWidget());
}

void
ChatWidgetHeader::leftButtonClicked()
{
    leftMenu.move(leftLabel.mapToGlobal(QPoint(0, leftLabel.height())));
    leftMenu.show();
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
