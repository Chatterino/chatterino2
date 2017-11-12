#include "widgets/helper/notebooktab.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"
#include "widgets/notebook.hpp"
#include "widgets/textinputdialog.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *_notebook)
    : BaseWidget(_notebook)
    , positionChangedAnimation(this, "pos")
    , notebook(_notebook)
    , menu(this)
{
    this->calcSize();
    this->setAcceptDrops(true);

    this->positionChangedAnimation.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));

    this->hideXConnection = SettingsManager::getInstance().hideTabX.valueChanged.connect(
        boost::bind(&NotebookTab::hideTabXChanged, this, _1));

    this->setMouseTracking(true);

    this->menu.addAction("Rename", [this]() {
        TextInputDialog d(this);

        d.setWindowTitle("Change tab title (Leave empty for default behaviour)");
        if (this->useDefaultBehaviour) {
            d.setText("");
        } else {
            d.setText(this->getTitle());
        }

        if (d.exec() == QDialog::Accepted) {
            QString newTitle = d.getText();
            if (newTitle.isEmpty()) {
                this->useDefaultBehaviour = true;
                this->page->refreshTitle();
            } else {
                this->useDefaultBehaviour = false;
                this->setTitle(newTitle);
            }
        }
    });

    this->menu.addAction("Close", [=]() {
        this->notebook->removePage(this->page);

        qDebug() << "lmoa";
    });

    this->menu.addAction("Enable highlights on new message", []() {
        qDebug() << "TODO: Implement";  //
    });
}

NotebookTab::~NotebookTab()
{
    this->hideXConnection.disconnect();
}

void NotebookTab::calcSize()
{
    float scale = getDpiMultiplier();

    if (SettingsManager::getInstance().hideTabX.get()) {
        this->resize(static_cast<int>((fontMetrics().width(title) + 16) * scale),
                     static_cast<int>(24 * scale));
    } else {
        this->resize(static_cast<int>((fontMetrics().width(title) + 8 + 24) * scale),
                     static_cast<int>(24 * scale));
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

    this->calcSize();
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
        if (this->window() == QApplication::activeWindow()) {
            painter.fillRect(rect(), this->colorScheme.TabSelectedBackground);
            fg = this->colorScheme.TabSelectedText;
        } else {
            painter.fillRect(rect(), this->colorScheme.TabSelectedUnfocusedBackground);
            fg = this->colorScheme.TabSelectedUnfocusedText;
        }
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

    float scale = this->getDpiMultiplier();
    int rectW = (SettingsManager::getInstance().hideTabX.get() ? 0 : static_cast<int>(16) * scale);
    QRect rect(0, 0, this->width() - rectW, this->height());

    painter.drawText(rect, title, QTextOption(Qt::AlignCenter));

    if (!SettingsManager::getInstance().hideTabX.get() && (mouseOver || selected)) {
        QRect xRect = this->getXRect();
        if (mouseOverX) {
            painter.fillRect(xRect, QColor(0, 0, 0, 64));

            if (mouseDownX) {
                painter.fillRect(xRect, QColor(0, 0, 0, 64));
            }
        }

        int a = static_cast<int>(scale * 4);

        painter.drawLine(xRect.topLeft() + QPoint(a, a), xRect.bottomRight() + QPoint(-a, -a));
        painter.drawLine(xRect.topRight() + QPoint(-a, a), xRect.bottomLeft() + QPoint(a, -a));
    }
}

void NotebookTab::mousePressEvent(QMouseEvent *event)
{
    this->mouseDown = true;
    this->mouseDownX = this->getXRect().contains(event->pos());

    this->update();

    this->notebook->select(page);

    switch (event->button()) {
        case Qt::RightButton: {
            this->menu.popup(event->globalPos());
        } break;
    }
}

void NotebookTab::mouseReleaseEvent(QMouseEvent *event)
{
    this->mouseDown = false;

    if (event->button() == Qt::MiddleButton) {
        if (this->rect().contains(event->pos())) {
            this->notebook->removePage(this->page);
        }
    } else {
        if (!SettingsManager::getInstance().hideTabX.get() && this->mouseDownX &&
            this->getXRect().contains(event->pos())) {
            this->mouseDownX = false;

            this->notebook->removePage(this->page);
        } else {
            this->update();
        }
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
        SplitContainer *clickedPage = notebook->tabAt(relPoint, index);

        if (clickedPage != nullptr && clickedPage != this->page) {
            this->notebook->rearrangePage(clickedPage, index);
        }
    }
}

void NotebookTab::load(const boost::property_tree::ptree &tree)
{
    // Load tab title
    try {
        QString newTitle = QString::fromStdString(tree.get<std::string>("title"));
        if (newTitle.isEmpty()) {
            this->useDefaultBehaviour = true;
        } else {
            this->setTitle(newTitle);
            this->useDefaultBehaviour = false;
        }
    } catch (boost::property_tree::ptree_error) {
    }
}

boost::property_tree::ptree NotebookTab::save()
{
    boost::property_tree::ptree tree;

    if (this->useDefaultBehaviour) {
        tree.put("title", "");
    } else {
        tree.put("title", this->getTitle().toStdString());
    }

    return tree;
}

}  // namespace widgets
}  // namespace chatterino
