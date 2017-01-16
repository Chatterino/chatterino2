#include "chatwidgetheader.h"
#include "chatwidget.h"
#include "colorscheme.h"
#include "notebookpage.h"

#include <QByteArray>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

ChatWidgetHeader::ChatWidgetHeader(ChatWidget *parent)
    : QWidget()
    , m_chatWidget(parent)
    , m_dragStart()
    , m_dragging(false)
    , m_leftLabel()
    , m_middleLabel()
    , m_rightLabel()
    , m_leftMenu(this)
    , m_rightMenu(this)
{
    setFixedHeight(32);

    updateColors();

    setLayout(&m_hbox);
    m_hbox.setMargin(0);
    m_hbox.addWidget(&m_leftLabel);
    m_hbox.addWidget(&m_middleLabel, 1);
    m_hbox.addWidget(&m_rightLabel);

    // left
    m_leftLabel.label().setTextFormat(Qt::RichText);
    m_leftLabel.label().setText(
        "<img src=':/images/tool_moreCollapser_off16.png' />");

    QObject::connect(&m_leftLabel, &ChatWidgetHeaderButton::clicked, this,
                     &ChatWidgetHeader::leftButtonClicked);

    //    leftMenu.addAction("Add new split", this,
    //    &ChatWidgetHeader::menuAddSplit,
    //                       QKeySequence(tr("Ctrl+T")));
    //    leftMenu.addAction("Close split", this,
    //    &ChatWidgetHeader::menuCloseSplit,
    //                       QKeySequence(tr("Ctrl+W")));
    //    leftMenu.addAction("Move split", this,
    //    &ChatWidgetHeader::menuMoveSplit);
    //    leftMenu.addSeparator();
    //    leftMenu.addAction("Change channel", this,
    //                       &ChatWidgetHeader::menuChangeChannel,
    //                       QKeySequence(tr("Ctrl+R")));
    //    leftMenu.addAction("Clear chat", this,
    //    &ChatWidgetHeader::menuClearChat);
    //    leftMenu.addAction("Open channel", this,
    //                       &ChatWidgetHeader::menuOpenChannel);
    //    leftMenu.addAction("Open pop-out player", this,
    //                       &ChatWidgetHeader::menuPopupPlayer);
    //    leftMenu.addSeparator();
    //    leftMenu.addAction("Reload channel emotes", this,
    //                       &ChatWidgetHeader::menuReloadChannelEmotes);
    //    leftMenu.addAction("Manual reconnect", this,
    //                       &ChatWidgetHeader::menuManualReconnect);
    //    leftMenu.addSeparator();
    //    leftMenu.addAction("Show changelog", this,
    //                       &ChatWidgetHeader::menuShowChangelog);

    // middle
    m_middleLabel.setAlignment(Qt::AlignCenter);
    m_middleLabel.setText("textString");

    // right
    m_rightLabel.setMinimumWidth(height());
    m_rightLabel.label().setTextFormat(Qt::RichText);
    m_rightLabel.label().setText("ayy");
}

void
ChatWidgetHeader::updateColors()
{
    QPalette palette;
    palette.setColor(QPalette::Foreground, ColorScheme::instance().Text);

    m_leftLabel.setPalette(palette);
    m_middleLabel.setPalette(palette);
    m_rightLabel.setPalette(palette);
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
            auto chatWidget = m_chatWidget;
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

void
ChatWidgetHeader::leftButtonClicked()
{
    m_leftMenu.move(m_leftLabel.mapToGlobal(QPoint(0, m_leftLabel.height())));
    m_leftMenu.show();
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
