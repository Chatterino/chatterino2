#include "widgets/notebooktab.h"
#include "colorscheme.h"
#include "settings/settings.h"
#include "widgets/notebook.h"

#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
    //    , posAnimation(this, "pos")
    //    , posAnimated(false)
    , notebook(notebook)
    , text("<no title>")
    , selected(false)
    , mouseOver(false)
    , mouseDown(false)
    , mouseOverX(false)
    , mouseDownX(false)
    , highlightStyle(HighlightNone)
{
    this->calcSize();
    this->setAcceptDrops(true);

    /* XXX(pajlada): Fix this
    QObject::connect(&settings::Settings::getInstance().getHideTabX(),
                     &settings::BoolSetting::valueChanged, this,
                     &NotebookTab::hideTabXChanged);
                     */

    this->setMouseTracking(true);
}

NotebookTab::~NotebookTab()
{
    /* XXX(pajlada): Fix this
    QObject::disconnect(&settings::Settings::getInstance().getHideTabX(),
                        &settings::BoolSetting::valueChanged, this,
                        &NotebookTab::hideTabXChanged);
                     */
}

void
NotebookTab::calcSize()
{
    if (settings::Settings::getInstance().hideTabX.get()) {
        this->resize(this->fontMetrics().width(this->text) + 8, 24);
    } else {
        this->resize(this->fontMetrics().width(this->text) + 8 + 24, 24);
    }
}

void
NotebookTab::moveAnimated(QPoint pos)
{
    move(pos);

    //    if (posAnimated == false) {
    //        move(pos);

    //        posAnimated = true;
    //        return;
    //    }

    //    if (this->posAnimation.endValue() == pos) {
    //        return;
    //    }

    //    this->posAnimation.stop();
    //    this->posAnimation.setDuration(50);
    //    this->posAnimation.setStartValue(this->pos());
    //    this->posAnimation.setEndValue(pos);
    //    this->posAnimation.start();
}

void
NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor fg = QColor(0, 0, 0);

    auto colorScheme = ColorScheme::instance();

    if (this->selected) {
        painter.fillRect(rect(), colorScheme.TabSelectedBackground);
        fg = colorScheme.TabSelectedText;
    } else if (this->mouseOver) {
        painter.fillRect(rect(), colorScheme.TabHoverBackground);
        fg = colorScheme.TabHoverText;
    } else if (this->highlightStyle == HighlightHighlighted) {
        painter.fillRect(rect(), colorScheme.TabHighlightedBackground);
        fg = colorScheme.TabHighlightedText;
    } else if (this->highlightStyle == HighlightNewMessage) {
        painter.fillRect(rect(), colorScheme.TabNewMessageBackground);
        fg = colorScheme.TabHighlightedText;
    } else {
        painter.fillRect(rect(), colorScheme.TabBackground);
        fg = colorScheme.TabText;
    }

    painter.setPen(fg);

    QRect rect(
        0, 0,
        width() - (settings::Settings::getInstance().hideTabX.get() ? 0 : 16),
        height());

    painter.drawText(rect, this->text, QTextOption(Qt::AlignCenter));

    if (!settings::Settings::getInstance().hideTabX.get() &&
        (this->mouseOver || this->selected)) {
        if (this->mouseOverX) {
            painter.fillRect(this->getXRect(), QColor(0, 0, 0, 64));

            if (this->mouseDownX) {
                painter.fillRect(this->getXRect(), QColor(0, 0, 0, 64));
            }
        }

        painter.drawLine(this->getXRect().topLeft() + QPoint(4, 4),
                         this->getXRect().bottomRight() + QPoint(-4, -4));
        painter.drawLine(this->getXRect().topRight() + QPoint(-4, 4),
                         this->getXRect().bottomLeft() + QPoint(4, -4));
    }
}

void
NotebookTab::mousePressEvent(QMouseEvent *event)
{
    this->mouseDown = true;
    this->mouseDownX = this->getXRect().contains(event->pos());

    this->repaint();

    this->notebook->select(page);
}

void
NotebookTab::mouseReleaseEvent(QMouseEvent *event)
{
    this->mouseDown = false;

    if (this->mouseDownX) {
        this->mouseDownX = false;

        this->notebook->removePage(this->page);
    } else {
        repaint();
    }
}

void
NotebookTab::enterEvent(QEvent *)
{
    this->mouseOver = true;

    repaint();
}

void
NotebookTab::leaveEvent(QEvent *)
{
    this->mouseOverX = this->mouseOver = false;

    repaint();
}

void
NotebookTab::dragEnterEvent(QDragEnterEvent *event)
{
    this->notebook->select(page);
}

void
NotebookTab::mouseMoveEvent(QMouseEvent *event)
{
    bool overX = this->getXRect().contains(event->pos());

    if (overX != this->mouseOverX) {
        this->mouseOverX = overX;

        this->repaint();
    }

    if (this->mouseDown && !this->rect().contains(event->pos())) {
        QPoint relPoint = this->mapToParent(event->pos());

        int index;
        NotebookPage *page = notebook->tabAt(relPoint, index);

        if (page != nullptr && page != this->page) {
            notebook->rearrangePage(this->page, index);
        }
    }
}
}
}
