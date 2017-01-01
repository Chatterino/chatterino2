#include "chatwidgetheader.h"
#include "QPainter"
#include "colorscheme.h"
#include "chatwidget.h"
#include "notebookpage.h"
#include "QDrag"
#include "QMimeData"
#include "QByteArray"

ChatWidgetHeader::ChatWidgetHeader()
    : QWidget()
{
    setFixedHeight(32);
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
    dragging = true;

    dragStart = event->pos();
}

void ChatWidgetHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging)
    {
        if (std::abs(dragStart.x() - event->pos().x()) > 12 ||
            std::abs(dragStart.y() - event->pos().y()) > 12)
        {
            auto chatWidget = getChatWidget();
            auto page = static_cast<NotebookPage*>(chatWidget->parentWidget());

            if (page != NULL)
            {
                NotebookPage::isDraggingSplit = true;
                NotebookPage::draggingSplit = chatWidget;

                auto originalLocation = page->removeFromLayout(chatWidget);

                //page->repaint();

                QDrag *drag = new QDrag(chatWidget);
                QMimeData* mimeData = new QMimeData;

                mimeData->setData("chatterino/split", "xD");

                drag->setMimeData(mimeData);

                Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

                if (dropAction == Qt::IgnoreAction)
                {
                    page->addToLayout(chatWidget, originalLocation);
                }

                NotebookPage::isDraggingSplit = false;
            }
        }
    }
}

ChatWidget* ChatWidgetHeader::getChatWidget()
{
    return static_cast<ChatWidget*>(parentWidget());
}
