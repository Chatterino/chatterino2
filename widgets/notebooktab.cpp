#include "widgets/notebooktab.h"
#include "colorscheme.h"
#include "settings.h"
#include "widgets/notebook.h"

#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
    , posAnimation(this, "pos")
    , posAnimated(false)
    , posAnimationDesired()
    , notebook(notebook)
    , title("<no title>")
    , selected(false)
    , mouseOver(false)
    , mouseDown(false)
    , mouseOverX(false)
    , mouseDownX(false)
    , highlightStyle(HighlightNone)
{
    this->calcSize();
    this->setAcceptDrops(true);

    posAnimation.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));

    this->hideXConnection =
        Settings::getInstance().hideTabX.valueChanged.connect(
            boost::bind(&NotebookTab::hideTabXChanged, this, _1));

    this->setMouseTracking(true);
}

NotebookTab::~NotebookTab()
{
    this->hideXConnection.disconnect();
}

void
NotebookTab::calcSize()
{
    if (Settings::getInstance().hideTabX.get()) {
        this->resize(this->fontMetrics().width(this->title) + 8, 24);
    } else {
        this->resize(this->fontMetrics().width(this->title) + 8 + 24, 24);
    }

    if (this->parent() != nullptr) {
        ((Notebook *)this->parent())->performLayout(true);
    }
}

void
NotebookTab::moveAnimated(QPoint pos, bool animated)
{
    posAnimationDesired = pos;

    if ((this->window() != NULL && !this->window()->isVisible()) || !animated ||
        posAnimated == false) {
        move(pos);

        posAnimated = true;
        return;
    }

    if (this->posAnimation.endValue() == pos) {
        return;
    }

    this->posAnimation.stop();
    this->posAnimation.setDuration(75);
    this->posAnimation.setStartValue(this->pos());
    this->posAnimation.setEndValue(pos);
    this->posAnimation.start();
}

void
NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor fg = QColor(0, 0, 0);

    auto &colorScheme = ColorScheme::getInstance();

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

    QRect rect(0, 0,
               width() - (Settings::getInstance().hideTabX.get() ? 0 : 16),
               height());

    painter.drawText(rect, this->title, QTextOption(Qt::AlignCenter));

    if (!Settings::getInstance().hideTabX.get() &&
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

    this->update();

    this->notebook->select(page);
}

void
NotebookTab::mouseReleaseEvent(QMouseEvent *event)
{
    this->mouseDown = false;

    if (!Settings::getInstance().hideTabX.get() && this->mouseDownX &&
        this->getXRect().contains(event->pos())) {
        this->mouseDownX = false;

        this->notebook->removePage(this->page);
    } else {
        update();
    }
}

void
NotebookTab::enterEvent(QEvent *)
{
    this->mouseOver = true;

    update();
}

void
NotebookTab::leaveEvent(QEvent *)
{
    this->mouseOverX = this->mouseOver = false;

    update();
}

void
NotebookTab::dragEnterEvent(QDragEnterEvent *)
{
    this->notebook->select(page);
}

void
NotebookTab::mouseMoveEvent(QMouseEvent *event)
{
    bool overX = this->getXRect().contains(event->pos());

    if (overX != this->mouseOverX) {
        this->mouseOverX = overX && !Settings::getInstance().hideTabX.get();

        this->update();
    }

    if (this->mouseDown && !this->getDesiredRect().contains(event->pos())) {
        QPoint relPoint = this->mapToParent(event->pos());

        int index;
        NotebookPage *page = notebook->tabAt(relPoint, index);

        if (page != nullptr && page != this->page) {
            notebook->rearrangePage(this->page, index);
        }
    }
}

void
NotebookTab::load(const boost::property_tree::ptree &tree)
{
    // Load tab title
    try {
        this->setTitle(QString::fromStdString(tree.get<std::string>("title")));
    } catch (boost::property_tree::ptree_error) {
    }
}

boost::property_tree::ptree
NotebookTab::save()
{
    boost::property_tree::ptree tree;

    tree.put("title", this->getTitle().toStdString());

    return tree;
}

}  // namespace widgets
}  // namespace chatterino
