#include "widgets/notebooktab.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"
#include "widgets/notebook.hpp"

#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *_notebook)
    : BaseWidget(_notebook)
    , positionChangedAnimation(this, "pos")
    , notebook(_notebook)
{
    this->calcSize();
    this->setAcceptDrops(true);

    this->positionChangedAnimation.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));

    this->hideXConnection = SettingsManager::getInstance().hideTabX.valueChanged.connect(
        boost::bind(&NotebookTab::hideTabXChanged, this, _1));

    this->setMouseTracking(true);
}

NotebookTab::~NotebookTab()
{
    this->hideXConnection.disconnect();
}

void NotebookTab::calcSize()
{
    if (SettingsManager::getInstance().hideTabX.get()) {
        this->resize(fontMetrics().width(title) + 8, 24);
    } else {
        this->resize(fontMetrics().width(title) + 8 + 24, 24);
    }

    if (this->parent() != nullptr) {
        (static_cast<Notebook *>(this->parent()))->performLayout(true);
    }
}

const QString &NotebookTab::getTitle() const
{
    return title;
}

void NotebookTab::setTitle(const QString &newTitle)
{
    this->title = newTitle;
}

bool NotebookTab::isSelected() const
{
    return this->selected;
}

void NotebookTab::setSelected(bool value)
{
    this->selected = value;

    this->update();
}

NotebookTab::HighlightStyle NotebookTab::getHighlightStyle() const
{
    return this->highlightStyle;
}

void NotebookTab::setHighlightStyle(HighlightStyle newHighlightStyle)
{
    this->highlightStyle = newHighlightStyle;

    this->update();
}

QRect NotebookTab::getDesiredRect() const
{
    return QRect(positionAnimationDesiredPoint, size());
}

void NotebookTab::hideTabXChanged(bool)
{
    this->calcSize();
    this->update();
}

void NotebookTab::moveAnimated(QPoint pos, bool animated)
{
    this->positionAnimationDesiredPoint = pos;

    QWidget *w = this->window();

    if ((w != nullptr && !w->isVisible()) || !animated || !positionChangedAnimationRunning) {
        this->move(pos);

        this->positionChangedAnimationRunning = true;
        return;
    }

    if (this->positionChangedAnimation.endValue() == pos) {
        return;
    }

    this->positionChangedAnimation.stop();
    this->positionChangedAnimation.setDuration(75);
    this->positionChangedAnimation.setStartValue(this->pos());
    this->positionChangedAnimation.setEndValue(pos);
    this->positionChangedAnimation.start();
}

void NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor fg = QColor(0, 0, 0);

    if (this->selected) {
        painter.fillRect(rect(), this->colorScheme.TabSelectedBackground);
        fg = this->colorScheme.TabSelectedText;
    } else if (this->mouseOver) {
        painter.fillRect(rect(), this->colorScheme.TabHoverBackground);
        fg = this->colorScheme.TabHoverText;
    } else if (this->highlightStyle == HighlightHighlighted) {
        painter.fillRect(rect(), this->colorScheme.TabHighlightedBackground);
        fg = this->colorScheme.TabHighlightedText;
    } else if (this->highlightStyle == HighlightNewMessage) {
        painter.fillRect(rect(), this->colorScheme.TabNewMessageBackground);
        fg = this->colorScheme.TabHighlightedText;
    } else {
        painter.fillRect(rect(), this->colorScheme.TabBackground);
        fg = this->colorScheme.TabText;
    }

    painter.setPen(fg);

    QRect rect(0, 0, this->width() - (SettingsManager::getInstance().hideTabX.get() ? 0 : 16),
               this->height());

    painter.drawText(rect, title, QTextOption(Qt::AlignCenter));

    if (!SettingsManager::getInstance().hideTabX.get() && (mouseOver || selected)) {
        QRect xRect = this->getXRect();
        if (mouseOverX) {
            painter.fillRect(xRect, QColor(0, 0, 0, 64));

            if (mouseDownX) {
                painter.fillRect(xRect, QColor(0, 0, 0, 64));
            }
        }

        painter.drawLine(xRect.topLeft() + QPoint(4, 4), xRect.bottomRight() + QPoint(-4, -4));
        painter.drawLine(xRect.topRight() + QPoint(-4, 4), xRect.bottomLeft() + QPoint(4, -4));
    }
}

void NotebookTab::mousePressEvent(QMouseEvent *event)
{
    this->mouseDown = true;
    this->mouseDownX = this->getXRect().contains(event->pos());

    this->update();

    this->notebook->select(page);
}

void NotebookTab::mouseReleaseEvent(QMouseEvent *event)
{
    this->mouseDown = false;

    if (!SettingsManager::getInstance().hideTabX.get() && this->mouseDownX &&
        this->getXRect().contains(event->pos())) {
        this->mouseDownX = false;

        this->notebook->removePage(this->page);
    } else {
        this->update();
    }
}

void NotebookTab::enterEvent(QEvent *)
{
    this->mouseOver = true;

    this->update();
}

void NotebookTab::leaveEvent(QEvent *)
{
    this->mouseOverX = false;
    this->mouseOver = false;

    this->update();
}

void NotebookTab::dragEnterEvent(QDragEnterEvent *)
{
    this->notebook->select(this->page);
}

void NotebookTab::mouseMoveEvent(QMouseEvent *event)
{
    if (!SettingsManager::getInstance().hideTabX.get()) {
        bool overX = this->getXRect().contains(event->pos());

        if (overX != this->mouseOverX) {
            // Over X state has been changed (we either left or entered it;
            this->mouseOverX = overX;

            this->update();
        }
    }

    if (this->mouseDown && !this->getDesiredRect().contains(event->pos())) {
        QPoint relPoint = this->mapToParent(event->pos());

        int index;
        NotebookPage *clickedPage = notebook->tabAt(relPoint, index);

        if (clickedPage != nullptr && clickedPage != this->page) {
            this->notebook->rearrangePage(clickedPage, index);
        }
    }
}

void NotebookTab::load(const boost::property_tree::ptree &tree)
{
    // Load tab title
    try {
        this->setTitle(QString::fromStdString(tree.get<std::string>("title")));
    } catch (boost::property_tree::ptree_error) {
    }
}

boost::property_tree::ptree NotebookTab::save()
{
    boost::property_tree::ptree tree;

    tree.put("title", this->getTitle().toStdString());

    return tree;
}

}  // namespace widgets
}  // namespace chatterino
