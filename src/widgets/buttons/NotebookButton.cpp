#include "widgets/buttons/NotebookButton.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/Theme.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/DraggedSplit.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>

namespace chatterino {

NotebookButton::NotebookButton(Type type_, Notebook *parent)
    : Button(parent)
    , parent_(parent)
    , type(type_)
{
    switch (this->type)
    {
        case Type::Plus: {
            this->setAcceptDrops(true);
        }
        break;
    }
}

void NotebookButton::themeChangedEvent()
{
    this->setMouseEffectColor(this->theme->tabs.regular.text);
}

void NotebookButton::paintContent(QPainter &painter)
{
    QColor background;
    QColor foreground;

    if (this->leftMouseButtonDown() || this->mouseOver())
    {
        background = this->theme->tabs.regular.backgrounds.hover;
        foreground = this->theme->tabs.regular.text;
    }
    else
    {
        background = this->theme->tabs.regular.backgrounds.regular;
        foreground = this->theme->tabs.regular.text;
    }

    painter.setPen(Qt::NoPen);

    float h = height(), w = width();

    switch (this->type)
    {
        case Type::Plus: {
            painter.setPen([&] {
                QColor tmp = foreground;
                if (isDraggingSplit())
                {
                    tmp = this->theme->tabs.selected.line.regular;
                }
                else if (!this->mouseOver())
                {
                    tmp.setAlpha(180);
                }
                return tmp;
            }());
            QRect rect = this->rect();
            int s = h * 4 / 9;

            painter.drawLine(rect.left() + rect.width() / 2 - (s / 2),
                             rect.top() + rect.height() / 2,
                             rect.left() + rect.width() / 2 + (s / 2),
                             rect.top() + rect.height() / 2);
            painter.drawLine(rect.left() + rect.width() / 2,
                             rect.top() + rect.height() / 2 - (s / 2),
                             rect.left() + rect.width() / 2,
                             rect.top() + rect.height() / 2 + (s / 2));
        }
        break;
    }
}

void NotebookButton::dragEnterEvent(QDragEnterEvent *event)
{
    if (this->type != Type::Plus)
    {
        return;
    }

    if (!event->mimeData()->hasFormat("chatterino/split"))
    {
        return;
    }

    event->acceptProposedAction();

    this->addClickEffect(QPoint{
        this->width() / 2,
        this->height() / 2,
    });
}

void NotebookButton::dropEvent(QDropEvent *event)
{
    if (this->type != Type::Plus)
    {
        return;
    }

    auto *draggedSplit = dynamic_cast<Split *>(event->source());
    if (!draggedSplit)
    {
        qCDebug(chatterinoWidget)
            << "Dropped something that wasn't a split onto a notebook button";
        return;
    }

    auto *notebook = dynamic_cast<Notebook *>(this->parentWidget());
    if (!notebook)
    {
        qCDebug(chatterinoWidget) << "Dropped a split onto a notebook button "
                                     "without a parent notebook";
        return;
    }

    event->acceptProposedAction();

    auto *page = new SplitContainer(notebook);
    auto *tab = notebook->addPage(page);
    page->setTab(tab);

    draggedSplit->setParent(page);
    page->insertSplit(draggedSplit);
}

void NotebookButton::hideEvent(QHideEvent *)
{
    if (isAppAboutToQuit())
    {
        return;
    }

    this->parent_->refresh();
}

void NotebookButton::showEvent(QShowEvent *)
{
    if (isAppAboutToQuit())
    {
        return;
    }

    this->parent_->refresh();
}

}  // namespace chatterino
