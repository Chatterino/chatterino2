#include "notebookpage.h"
#include "QHBoxLayout"
#include "QMimeData"
#include "QPainter"
#include "QVBoxLayout"
#include "QWidget"
#include "chatwidget.h"
#include "colorscheme.h"
#include "notebooktab.h"

bool NotebookPage::isDraggingSplit = false;
ChatWidget *NotebookPage::draggingSplit = NULL;
std::pair<int, int> NotebookPage::dropPosition = std::pair<int, int>(-1, -1);

NotebookPage::NotebookPage(QWidget *parent, NotebookTab *tab)
    : QWidget(parent)
    , parentbox(this)
    , preview(this)
{
    this->tab = tab;
    tab->page = this;

    setHidden(true);
    setAcceptDrops(true);

    parentbox.addSpacing(2);
    parentbox.addLayout(&hbox);
    parentbox.setMargin(0);

    hbox.setSpacing(1);
    hbox.setMargin(0);
}

std::pair<int, int>
NotebookPage::removeFromLayout(ChatWidget *widget)
{
    for (int i = 0; i < hbox.count(); ++i) {
        auto vbox = static_cast<QVBoxLayout *>(hbox.itemAt(i));

        for (int j = 0; j < vbox->count(); ++j) {
            if (vbox->itemAt(j)->widget() != widget)
                continue;

            widget->setParent(NULL);

            bool isLastItem = vbox->count() == 0;

            if (isLastItem) {
                hbox.removeItem(vbox);

                delete vbox;
            }

            return std::pair<int, int>(i, isLastItem ? -1 : j);
            ;
        }
    }

    return std::pair<int, int>(-1, -1);
}

void
NotebookPage::addToLayout(
    ChatWidget *widget,
    std::pair<int, int> position = std::pair<int, int>(-1, -1))
{
    // add vbox at the end
    if (position.first < 0 || position.first >= hbox.count()) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        hbox.addLayout(vbox);
        return;
    }

    // insert vbox
    if (position.second == -1) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        hbox.insertLayout(position.first, vbox);
        return;
    }

    // add to existing vbox
    auto vbox = static_cast<QVBoxLayout *>(hbox.itemAt(position.first));

    vbox->insertWidget(std::max(0, std::min(vbox->count(), position.second)),
                       widget);
}

void
NotebookPage::enterEvent(QEvent *)
{
    if (hbox.count() == 0) {
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
    if (hbox.count() == 0 && event->button() == Qt::LeftButton) {
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
        dropRegions.clear();

        if (hbox.count() == 0) {
            dropRegions.push_back(
                DropRegion(rect(), std::pair<int, int>(-1, -1)));
        } else {
            for (int i = 0; i < hbox.count() + 1; ++i) {
                dropRegions.push_back(
                    DropRegion(QRect(((i * 4 - 1) * width() / hbox.count()) / 4,
                                     0, width() / hbox.count() / 2, height()),
                               std::pair<int, int>(i, -1)));
            }

            for (int i = 0; i < hbox.count(); ++i) {
                auto vbox = static_cast<QVBoxLayout *>(hbox.itemAt(i));

                for (int j = 0; j < vbox->count() + 1; ++j) {
                    dropRegions.push_back(DropRegion(
                        QRect(i * width() / hbox.count(),
                              ((j * 2 - 1) * height() / vbox->count()) / 2,
                              width() / hbox.count(), height() / vbox->count()),
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
    for (DropRegion region : dropRegions) {
        if (region.rect.contains(mousePos)) {
            preview.move(region.rect.x(), region.rect.y());
            preview.resize(region.rect.width(), region.rect.height());
            preview.show();
            preview.raise();

            dropPosition = region.position;

            return;
        } else {
            preview.hide();
        }
    }
}

void
NotebookPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    preview.hide();
}

void
NotebookPage::dropEvent(QDropEvent *event)
{
    if (isDraggingSplit) {
        event->acceptProposedAction();

        NotebookPage::draggingSplit->setParent(this);

        addToLayout(NotebookPage::draggingSplit, dropPosition);
    }

    preview.hide();
}

void
NotebookPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (hbox.count() == 0) {
        painter.fillRect(rect(), ColorScheme::instance().ChatBackground);

        painter.fillRect(0, 0, width(), 2,
                         ColorScheme::instance().TabSelectedBackground);

        painter.setPen(ColorScheme::instance().Text);
        painter.drawText(rect(), "Add Chat", QTextOption(Qt::AlignCenter));
    } else {
        painter.fillRect(rect(), ColorScheme::instance().TabSelectedBackground);

        painter.fillRect(0, 0, width(), 2,
                         ColorScheme::instance().TabSelectedBackground);
    }
}
