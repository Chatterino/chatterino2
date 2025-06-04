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

NotebookButton::NotebookButton(Notebook *parent)
    : Button(parent)
    , parent_(parent)
{
    this->setAcceptDrops(true);
}

void NotebookButton::setIcon(Icon icon)
{
    this->icon_ = icon;

    this->update();
}

NotebookButton::Icon NotebookButton::getIcon() const
{
    return this->icon_;
}

void NotebookButton::themeChangedEvent()
{
    this->setMouseEffectColor(this->theme->tabs.regular.text);
}

void NotebookButton::paintContent(QPainter &painter)
{
    QColor background;
    QColor foreground;

    if (this->mouseDown() || this->mouseOver())
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

    switch (icon_)
    {
        case Plus: {
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

        case User: {
            painter.setRenderHint(QPainter::Antialiasing);

            auto a = w / 8;
            QPainterPath path;

            path.arcMoveTo(a, 4 * a, 6 * a, 6 * a, 0);
            path.arcTo(a, 4 * a, 6 * a, 6 * a, 0, 180);

            QPainterPath remove;
            remove.addEllipse(2 * a, 1 * a, 4 * a, 4 * a);
            path = path.subtracted(remove);

            path.addEllipse(2.5 * a, 1.5 * a, 3 * a, 3 * a);

            painter.fillPath(path, foreground);
        }
        break;

        case Settings: {
            painter.setRenderHint(QPainter::Antialiasing);

            auto a = w / 8;
            QPainterPath path;

            path.arcMoveTo(a, a, 6 * a, 6 * a, 0 - (360 / 32.0));

            for (int i = 0; i < 8; i++)
            {
                path.arcTo(a, a, 6 * a, 6 * a, i * (360 / 8.0) - (360 / 32.0),
                           (360 / 32.0));
                path.arcTo(2 * a, 2 * a, 4 * a, 4 * a,
                           i * (360 / 8.0) + (360 / 32.0), (360 / 32.0));
            }

            QPainterPath remove;
            remove.addEllipse(3 * a, 3 * a, 2 * a, 2 * a);

            painter.fillPath(path.subtracted(remove), foreground);
        }
        break;

        default:;
    }
}

void NotebookButton::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
    {
        return;
    }

    event->acceptProposedAction();

    auto *e =
        new QMouseEvent(QMouseEvent::MouseButtonPress,
                        QPointF(this->width() / 2, this->height() / 2),
                        QCursor::pos(), Qt::LeftButton, Qt::LeftButton, {});
    Button::mousePressEvent(e);
    delete e;
}

void NotebookButton::dragLeaveEvent(QDragLeaveEvent *)
{
    this->update();

    auto *e =
        new QMouseEvent(QMouseEvent::MouseButtonRelease,
                        QPointF(this->width() / 2, this->height() / 2),
                        QCursor::pos(), Qt::LeftButton, Qt::LeftButton, {});
    Button::mouseReleaseEvent(e);
    delete e;
}

void NotebookButton::dropEvent(QDropEvent *event)
{
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
