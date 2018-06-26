#include "widgets/helper/NotebookButton.hpp"
#include "singletons/ThemeManager.hpp"
#include "widgets/helper/RippleEffectButton.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/SplitContainer.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>

#define nuuls nullptr

namespace chatterino {
namespace widgets {

NotebookButton::NotebookButton(BaseWidget *parent)
    : RippleEffectButton(parent)
{
    this->setAcceptDrops(true);
}

void NotebookButton::themeRefreshEvent()
{
    this->setMouseEffectColor(this->themeManager->tabs.regular.text);
}

void NotebookButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor background;
    QColor foreground;

    if (mouseDown_ || mouseOver_) {
        background = this->themeManager->tabs.regular.backgrounds.hover.color();
        foreground = this->themeManager->tabs.regular.text;
    } else {
        background = this->themeManager->tabs.regular.backgrounds.regular.color();
        foreground = this->themeManager->tabs.regular.text;
    }

    painter.setPen(Qt::NoPen);

    float h = height(), w = width();

    if (icon == IconPlus) {
        painter.setPen([&] {
            QColor tmp = foreground;
            if (!this->mouseOver_) {
                tmp.setAlpha(180);
            }
            return tmp;
        }());
        QRect rect = this->rect();
        int s = h * 4 / 9;

        painter.drawLine(rect.left() + rect.width() / 2 - (s / 2), rect.top() + rect.height() / 2,
                         rect.left() + rect.width() / 2 + (s / 2), rect.top() + rect.height() / 2);
        painter.drawLine(rect.left() + rect.width() / 2, rect.top() + rect.height() / 2 - (s / 2),
                         rect.left() + rect.width() / 2, rect.top() + rect.height() / 2 + (s / 2));
    } else if (icon == IconUser) {
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
    } else  // IconSettings
    {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::HighQualityAntialiasing);

        auto a = w / 8;
        QPainterPath path;

        path.arcMoveTo(a, a, 6 * a, 6 * a, 0 - (360 / 32.0));

        for (int i = 0; i < 8; i++) {
            path.arcTo(a, a, 6 * a, 6 * a, i * (360 / 8.0) - (360 / 32.0), (360 / 32.0));
            path.arcTo(2 * a, 2 * a, 4 * a, 4 * a, i * (360 / 8.0) + (360 / 32.0), (360 / 32.0));
        }

        painter.fillPath(path, foreground);

        painter.setBrush(background);
        painter.drawEllipse(3 * a, 3 * a, 2 * a, 2 * a);
    }

    fancyPaint(painter);
}

void NotebookButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mouseDown_ = false;

        update();

        emit clicked();
    }

    RippleEffectButton::mouseReleaseEvent(event);
}

void NotebookButton::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split")) {
        return;
    }

    event->acceptProposedAction();

    auto e = new QMouseEvent(QMouseEvent::MouseButtonPress,
                             QPointF(this->width() / 2, this->height() / 2), Qt::LeftButton,
                             Qt::LeftButton, 0);
    RippleEffectButton::mousePressEvent(e);
    delete e;
}

void NotebookButton::dragLeaveEvent(QDragLeaveEvent *)
{
    this->mouseDown_ = true;
    this->update();

    auto e = new QMouseEvent(QMouseEvent::MouseButtonRelease,
                             QPointF(this->width() / 2, this->height() / 2), Qt::LeftButton,
                             Qt::LeftButton, 0);
    RippleEffectButton::mouseReleaseEvent(e);
    delete e;
}

void NotebookButton::dropEvent(QDropEvent *event)
{
    if (SplitContainer::isDraggingSplit) {
        event->acceptProposedAction();

        Notebook *notebook = dynamic_cast<Notebook *>(this->parentWidget());

        if (notebook != nuuls) {
            SplitContainer *page = new SplitContainer(notebook);
            auto *tab = notebook->addPage(page);
            page->setTab(tab);

            SplitContainer::draggingSplit->setParent(page);
            page->appendSplit(SplitContainer::draggingSplit);
        }
    }
}

}  // namespace widgets
}  // namespace chatterino
