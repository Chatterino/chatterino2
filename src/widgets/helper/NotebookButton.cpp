#include "widgets/helper/NotebookButton.hpp"
#include "singletons/Theme.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/helper/Button.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>

#define nuuls nullptr

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

void NotebookButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    QColor background;
    QColor foreground;

    if (mouseDown_ || mouseOver_)
    {
        background = this->theme->tabs.regular.backgrounds.hover.color();
        foreground = this->theme->tabs.regular.text;
    }
    else
    {
        background = this->theme->tabs.regular.backgrounds.regular.color();
        foreground = this->theme->tabs.regular.text;
    }

    painter.setPen(Qt::NoPen);

    float h = height(), w = width();

    switch (icon_)
    {
        case Plus:
        {
            painter.setPen([&] {
                QColor tmp = foreground;
                if (SplitContainer::isDraggingSplit)
                {
                    tmp = this->theme->tabs.selected.line.regular;
                }
                else if (!this->mouseOver_)
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

        case User:
        {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::HighQualityAntialiasing);

            auto a = w / 8;
            QPainterPath path;

            path.arcMoveTo(a, 4 * a, 6 * a, 6 * a, 0);
            path.arcTo(a, 4 * a, 6 * a, 6 * a, 0, 180);

            painter.fillPath(path, foreground);

            painter.setBrush(background);
            painter.drawEllipse(2 * a, 1 * a, 4 * a, 4 * a);

            painter.setBrush(foreground);
            painter.drawEllipse(2.5 * a, 1.5 * a, 3 * a + 1, 3 * a);
        }
        break;

        case Settings:
        {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::HighQualityAntialiasing);

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

            painter.fillPath(path, foreground);

            painter.setBrush(background);
            painter.drawEllipse(3 * a, 3 * a, 2 * a, 2 * a);
        }
        break;

        default:;
    }

    Button::paintEvent(event);
}

void NotebookButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        mouseDown_ = false;

        update();

        emit leftClicked();
    }

    Button::mouseReleaseEvent(event);
}

void NotebookButton::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
    {
        return;
    }

    event->acceptProposedAction();

    auto e = new QMouseEvent(QMouseEvent::MouseButtonPress,
                             QPointF(this->width() / 2, this->height() / 2),
                             Qt::LeftButton, Qt::LeftButton, 0);
    Button::mousePressEvent(e);
    delete e;
}

void NotebookButton::dragLeaveEvent(QDragLeaveEvent *)
{
    this->mouseDown_ = true;
    this->update();

    auto e = new QMouseEvent(QMouseEvent::MouseButtonRelease,
                             QPointF(this->width() / 2, this->height() / 2),
                             Qt::LeftButton, Qt::LeftButton, 0);
    Button::mouseReleaseEvent(e);
    delete e;
}

void NotebookButton::dropEvent(QDropEvent *event)
{
    if (SplitContainer::isDraggingSplit)
    {
        event->acceptProposedAction();

        Notebook *notebook = dynamic_cast<Notebook *>(this->parentWidget());

        if (notebook != nuuls)
        {
            SplitContainer *page = new SplitContainer(notebook);
            auto *tab = notebook->addPage(page);
            page->setTab(tab);

            SplitContainer::draggingSplit->setParent(page);
            page->appendSplit(SplitContainer::draggingSplit);
        }
    }
}

void NotebookButton::hideEvent(QHideEvent *)
{
    this->parent_->performLayout();
}

void NotebookButton::showEvent(QShowEvent *)
{
    this->parent_->performLayout();
}

}  // namespace chatterino
