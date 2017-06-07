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
    , _chatWidget(parent)
    , _dragStart()
    , _dragging(false)
    , _leftLabel()
    , _middleLabel()
    , _rightLabel()
    , _leftMenu(this)
    , _rightMenu(this)
{
    setFixedHeight(32);

    updateColors();
    updateChannelText();

    setLayout(&_hbox);
    _hbox.setMargin(0);
    _hbox.addWidget(&_leftLabel);
    _hbox.addWidget(&_middleLabel, 1);
    _hbox.addWidget(&_rightLabel);

    // left
    _leftLabel.getLabel().setTextFormat(Qt::RichText);
    _leftLabel.getLabel().setText("<img src=':/images/tool_moreCollapser_off16.png' />");

    QObject::connect(&_leftLabel, &ChatWidgetHeaderButton::clicked, this,
                     &ChatWidgetHeader::leftButtonClicked);

    _leftMenu.addAction("Add new split", this, SLOT(menuAddSplit()), QKeySequence(tr("Ctrl+T")));
    _leftMenu.addAction("Close split", this, SLOT(menuCloseSplit()), QKeySequence(tr("Ctrl+W")));
    _leftMenu.addAction("Move split", this, SLOT(menuMoveSplit()));
    _leftMenu.addAction("Popup", this, SLOT(menuPopup()));
    _leftMenu.addSeparator();
    _leftMenu.addAction("Change channel", this, SLOT(menuChangeChannel()),
                        QKeySequence(tr("Ctrl+R")));
    _leftMenu.addAction("Clear chat", this, SLOT(menuClearChat()));
    _leftMenu.addAction("Open channel", this, SLOT(menuOpenChannel()));
    _leftMenu.addAction("Open pop-out player", this, SLOT(menuPopupPlayer()));
    _leftMenu.addSeparator();
    _leftMenu.addAction("Reload channel emotes", this, SLOT(menuReloadChannelEmotes()));
    _leftMenu.addAction("Manual reconnect", this, SLOT(menuManualReconnect()));
    _leftMenu.addSeparator();
    _leftMenu.addAction("Show changelog", this, SLOT(menuShowChangelog()));

    // middle
    _middleLabel.setAlignment(Qt::AlignCenter);

    connect(&_middleLabel, &SignalLabel::mouseDoubleClick, this,
            &ChatWidgetHeader::mouseDoubleClickEvent);
    //    connect(&this->middleLabel, &SignalLabel::mouseDown, this,
    //            &ChatWidgetHeader::mouseDoubleClickEvent);

    // right
    _rightLabel.setMinimumWidth(height());
    _rightLabel.getLabel().setTextFormat(Qt::RichText);
    _rightLabel.getLabel().setText("ayy");
}

void ChatWidgetHeader::updateColors()
{
    QPalette palette;
    palette.setColor(QPalette::Foreground, ColorScheme::getInstance().Text);

    _leftLabel.setPalette(palette);
    _middleLabel.setPalette(palette);
    _rightLabel.setPalette(palette);
}

void ChatWidgetHeader::updateChannelText()
{
    const QString &c = _chatWidget->getChannelName();

    _middleLabel.setText(c.isEmpty() ? "<no channel>" : c);
}

void ChatWidgetHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::getInstance().ChatHeaderBackground);
    painter.setPen(ColorScheme::getInstance().ChatHeaderBorder);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void ChatWidgetHeader::mousePressEvent(QMouseEvent *event)
{
    _dragging = true;

    _dragStart = event->pos();
}

void ChatWidgetHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (_dragging) {
        if (std::abs(_dragStart.x() - event->pos().x()) > 12 ||
            std::abs(_dragStart.y() - event->pos().y()) > 12) {
            auto chatWidget = _chatWidget;
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

void ChatWidgetHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        _chatWidget->showChangeChannelPopup();
    }
}

void ChatWidgetHeader::leftButtonClicked()
{
    _leftMenu.move(_leftLabel.mapToGlobal(QPoint(0, _leftLabel.height())));
    _leftMenu.show();
}

void ChatWidgetHeader::rightButtonClicked()
{
}

void ChatWidgetHeader::menuAddSplit()
{
    auto page = static_cast<NotebookPage *>(_chatWidget->parentWidget());
    page->addChat();
}
void ChatWidgetHeader::menuCloseSplit()
{
    printf("Close split\n");
}
void ChatWidgetHeader::menuMoveSplit()
{
}
void ChatWidgetHeader::menuPopup()
{
    auto widget = new ChatWidget();
    widget->setChannelName(_chatWidget->getChannelName());
    widget->show();
}
void ChatWidgetHeader::menuChangeChannel()
{
    _chatWidget->showChangeChannelPopup();
}
void ChatWidgetHeader::menuClearChat()
{
}
void ChatWidgetHeader::menuOpenChannel()
{
}
void ChatWidgetHeader::menuPopupPlayer()
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
