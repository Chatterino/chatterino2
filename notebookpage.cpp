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
    , m_parentbox(this)
    , m_preview(this)
    , m_chatWidgets()
{
    this->tab = tab;
    tab->page = this;

    setHidden(true);
    setAcceptDrops(true);

    m_parentbox.addSpacing(2);
    m_parentbox.addLayout(&m_hbox);
    m_parentbox.setMargin(0);

    m_hbox.setSpacing(1);
    m_hbox.setMargin(0);
}

std::pair<int, int>
NotebookPage::removeFromLayout(ChatWidget *widget)
{
    for (auto it = m_chatWidgets.begin(); it != m_chatWidgets.end(); ++it) {
        if (*it == widget) {
            m_chatWidgets.erase(it);

            break;
        }
    }

    for (int i = 0; i < m_hbox.count(); ++i) {
        auto vbox = static_cast<QVBoxLayout *>(m_hbox.itemAt(i));

        for (int j = 0; j < vbox->count(); ++j) {
            if (vbox->itemAt(j)->widget() != widget)
                continue;

            widget->setParent(NULL);

            bool isLastItem = vbox->count() == 0;

            if (isLastItem) {
                m_hbox.removeItem(vbox);

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
    m_chatWidgets.push_back(widget);

    // add vbox at the end
    if (position.first < 0 || position.first >= m_hbox.count()) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        m_hbox.addLayout(vbox);
        return;
    }

    // insert vbox
    if (position.second == -1) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        m_hbox.insertLayout(position.first, vbox);
        return;
    }

    // add to existing vbox
    auto vbox = static_cast<QVBoxLayout *>(m_hbox.itemAt(position.first));

    vbox->insertWidget(std::max(0, std::min(vbox->count(), position.second)),
                       widget);
}

void
NotebookPage::enterEvent(QEvent *)
{
    if (m_hbox.count() == 0) {
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
    if (m_hbox.count() == 0 && event->button() == Qt::LeftButton) {
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
        m_dropRegions.clear();

        if (m_hbox.count() == 0) {
            m_dropRegions.push_back(
                DropRegion(rect(), std::pair<int, int>(-1, -1)));
        } else {
            for (int i = 0; i < m_hbox.count() + 1; ++i) {
                m_dropRegions.push_back(DropRegion(
                    QRect(((i * 4 - 1) * width() / m_hbox.count()) / 4, 0,
                          width() / m_hbox.count() / 2, height()),
                    std::pair<int, int>(i, -1)));
            }

            for (int i = 0; i < m_hbox.count(); ++i) {
                auto vbox = static_cast<QVBoxLayout *>(m_hbox.itemAt(i));

                for (int j = 0; j < vbox->count() + 1; ++j) {
                    m_dropRegions.push_back(DropRegion(
                        QRect(i * width() / m_hbox.count(),
                              ((j * 2 - 1) * height() / vbox->count()) / 2,
                              width() / m_hbox.count(),
                              height() / vbox->count()),
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
    for (DropRegion region : m_dropRegions) {
        if (region.rect.contains(mousePos)) {
            m_preview.setBounds(region.rect);
            //            m_preview.move(region.rect.x(), region.rect.y());
            //            m_preview.resize(region.rect.width(),
            //            region.rect.height());
            m_preview.show();
            m_preview.raise();

            dropPosition = region.position;

            return;
        } else {
            m_preview.hide();
        }
    }
}

void
NotebookPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_preview.hide();
}

void
NotebookPage::dropEvent(QDropEvent *event)
{
    if (isDraggingSplit) {
        event->acceptProposedAction();

        NotebookPage::draggingSplit->setParent(this);

        addToLayout(NotebookPage::draggingSplit, dropPosition);
    }

    m_preview.hide();
}

void
NotebookPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (m_hbox.count() == 0) {
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
