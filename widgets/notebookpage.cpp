#include "widgets/notebookpage.h"
#include "colorscheme.h"
#include "widgets/chatwidget.h"
#include "widgets/notebooktab.h"

#include <QHBoxLayout>
#include <QMimeData>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

namespace chatterino {
namespace widgets {

bool NotebookPage::isDraggingSplit = false;
ChatWidget *NotebookPage::draggingSplit = NULL;
std::pair<int, int> NotebookPage::dropPosition = std::pair<int, int>(-1, -1);

NotebookPage::NotebookPage(QWidget *parent, NotebookTab *tab)
    : QWidget(parent)
    , parentbox(this)
    , chatWidgets()
    , preview(this)
{
    this->tab = tab;
    tab->page = this;

    setHidden(true);
    setAcceptDrops(true);

    this->parentbox.addSpacing(2);
    this->parentbox.addLayout(&this->hbox);
    this->parentbox.setMargin(0);

    this->hbox.setSpacing(1);
    this->hbox.setMargin(0);
}

std::pair<int, int>
NotebookPage::removeFromLayout(ChatWidget *widget)
{
    for (auto it = this->chatWidgets.begin(); it != this->chatWidgets.end();
         ++it) {
        if (*it == widget) {
            this->chatWidgets.erase(it);

            break;
        }
    }

    for (int i = 0; i < this->hbox.count(); ++i) {
        auto vbox = static_cast<QVBoxLayout *>(this->hbox.itemAt(i));

        for (int j = 0; j < vbox->count(); ++j) {
            if (vbox->itemAt(j)->widget() != widget)
                continue;

            widget->setParent(NULL);

            bool isLastItem = vbox->count() == 0;

            if (isLastItem) {
                this->hbox.removeItem(vbox);

                delete vbox;
            }

            return std::pair<int, int>(i, isLastItem ? -1 : j);
        }
    }

    return std::pair<int, int>(-1, -1);
}

void
NotebookPage::addToLayout(
    ChatWidget *widget,
    std::pair<int, int> position = std::pair<int, int>(-1, -1))
{
    this->chatWidgets.push_back(widget);

    // add vbox at the end
    if (position.first < 0 || position.first >= this->hbox.count()) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        this->hbox.addLayout(vbox);
        return;
    }

    // insert vbox
    if (position.second == -1) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        this->hbox.insertLayout(position.first, vbox);
        return;
    }

    // add to existing vbox
    auto vbox = static_cast<QVBoxLayout *>(this->hbox.itemAt(position.first));

    vbox->insertWidget(std::max(0, std::min(vbox->count(), position.second)),
                       widget);
}

void
NotebookPage::enterEvent(QEvent *)
{
    if (this->hbox.count() == 0) {
        setCursor(QCursor(Qt::PointingHandCursor));
    } else {
        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void
NotebookPage::leaveEvent(QEvent *)
{
}

void
NotebookPage::mouseReleaseEvent(QMouseEvent *event)
{
    if (this->hbox.count() == 0 && event->button() == Qt::LeftButton) {
        addToLayout(new ChatWidget(), std::pair<int, int>(-1, -1));

        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void
NotebookPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
        return;

    if (isDraggingSplit) {
        this->dropRegions.clear();

        if (this->hbox.count() == 0) {
            this->dropRegions.push_back(
                DropRegion(rect(), std::pair<int, int>(-1, -1)));
        } else {
            for (int i = 0; i < this->hbox.count() + 1; ++i) {
                this->dropRegions.push_back(DropRegion(
                    QRect(((i * 4 - 1) * width() / this->hbox.count()) / 4, 0,
                          width() / this->hbox.count() / 2 + 1, height() + 1),

                    std::pair<int, int>(i, -1)));
            }

            for (int i = 0; i < this->hbox.count(); ++i) {
                auto vbox = static_cast<QVBoxLayout *>(this->hbox.itemAt(i));

                for (int j = 0; j < vbox->count() + 1; ++j) {
                    this->dropRegions.push_back(DropRegion(
                        QRect(i * width() / this->hbox.count(),
                              ((j * 2 - 1) * height() / vbox->count()) / 2,
                              width() / this->hbox.count() + 1,
                              height() / vbox->count() + 1),

                        std::pair<int, int>(i, j)));
                }
            }
        }

        setPreviewRect(event->pos());

        event->acceptProposedAction();
    }
}

void
NotebookPage::dragMoveEvent(QDragMoveEvent *event)
{
    setPreviewRect(event->pos());
}

void
NotebookPage::setPreviewRect(QPoint mousePos)
{
    for (DropRegion region : this->dropRegions) {
        if (region.rect.contains(mousePos)) {
            this->preview.setBounds(region.rect);

            if (!this->preview.isVisible()) {
                this->preview.show();
                this->preview.raise();
            }

            dropPosition = region.position;

            return;
        }
    }

    this->preview.hide();
}

void
NotebookPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    this->preview.hide();
}

void
NotebookPage::dropEvent(QDropEvent *event)
{
    if (isDraggingSplit) {
        event->acceptProposedAction();

        NotebookPage::draggingSplit->setParent(this);

        addToLayout(NotebookPage::draggingSplit, dropPosition);
    }

    this->preview.hide();
}

void
NotebookPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (this->hbox.count() == 0) {
        painter.fillRect(rect(), ColorScheme::getInstance().ChatBackground);

        painter.fillRect(0, 0, width(), 2,
                         ColorScheme::getInstance().TabSelectedBackground);

        painter.setPen(ColorScheme::getInstance().Text);
        painter.drawText(rect(), "Add Chat", QTextOption(Qt::AlignCenter));
    } else {
        painter.fillRect(rect(),
                         ColorScheme::getInstance().TabSelectedBackground);

        painter.fillRect(0, 0, width(), 2,
                         ColorScheme::getInstance().TabSelectedBackground);
    }
}
}
}
