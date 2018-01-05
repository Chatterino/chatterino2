//#include "widgets/helper/notebooktab.hpp"
#include "common.hpp"
#include "debug/log.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "util/helpers.hpp"
#include "widgets/notebook.hpp"
#include "widgets/settingsdialog.hpp"
#include "widgets/textinputdialog.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *_notebook, const std::string &_uuid)
    : BaseWidget(_notebook)
    , uuid(_uuid)
    , settingRoot(fS("/containers/{}/tab", this->uuid))
    , positionChangedAnimation(this, "pos")
    , notebook(_notebook)
    , title(fS("{}/title", this->settingRoot), "")
    , useDefaultBehaviour(fS("{}/useDefaultBehaviour", this->settingRoot), true)
    , menu(this)
{
    this->calcSize();
    this->setAcceptDrops(true);

    this->positionChangedAnimation.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));

    singletons::SettingManager::getInstance().hideTabX.connect(
        boost::bind(&NotebookTab::hideTabXChanged, this, _1), this->managedConnections);

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

    QAction *enableHighlightsOnNewMessageAction =
        new QAction("Enable highlights on new message", &this->menu);
    enableHighlightsOnNewMessageAction->setCheckable(true);

    this->menu.addAction("Close", [=]() { this->notebook->removePage(this->page); });

    this->menu.addAction(enableHighlightsOnNewMessageAction);

    connect(enableHighlightsOnNewMessageAction, &QAction::toggled, [this](bool newValue) {
        debug::Log("New value is {}", newValue);  //
    });
}

void NotebookTab::calcSize()
{
    float scale = getDpiMultiplier();
    QString qTitle(qS(this->title));

    if (singletons::SettingManager::getInstance().hideTabX) {
        this->resize(static_cast<int>((fontMetrics().width(qTitle) + 16) * scale),
                     static_cast<int>(24 * scale));
    } else {
        this->resize(static_cast<int>((fontMetrics().width(qTitle) + 8 + 24) * scale),
                     static_cast<int>(24 * scale));
    }

    if (this->parent() != nullptr) {
        (static_cast<Notebook *>(this->parent()))->performLayout(true);
    }
}

QString NotebookTab::getTitle() const
{
    return qS(this->title);
}

void NotebookTab::setTitle(const QString &newTitle)
{
    this->title = newTitle.toStdString();

    this->calcSize();
}

bool NotebookTab::isSelected() const
{
    return this->selected;
}

void NotebookTab::setSelected(bool value)
{
    this->selected = value;

    this->highlightState = HighlightState::None;

    this->update();
}

void NotebookTab::setHighlightState(HighlightState newHighlightStyle)
{
    if (this->isSelected()) {
        return;
    }

    this->highlightState = newHighlightStyle;

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
    singletons::SettingManager &settingManager = singletons::SettingManager::getInstance();
    QPainter painter(this);

    // select the right tab colors
    singletons::ThemeManager::TabColors colors;

    if (this->selected) {
        colors = this->themeManager.tabs.selected;
    } else if (this->highlightState == HighlightState::Highlighted) {
        colors = this->themeManager.tabs.highlighted;
    } else if (this->highlightState == HighlightState::NewMessage) {
        colors = this->themeManager.tabs.newMessage;
    } else {
        colors = this->themeManager.tabs.regular;
    }

    bool windowFocused = this->window() == QApplication::activeWindow();
    // || SettingsDialog::getHandle() == QApplication::activeWindow();

    // fill the tab background
    painter.fillRect(rect(),
                     this->mouseOver ? colors.backgrounds.hover
                                     : (windowFocused ? colors.backgrounds.regular
                                                      : colors.backgrounds.unfocused));

    // set the pen color
    painter.setPen(colors.text);

    // set area for text
    float scale = this->getDpiMultiplier();
    int rectW = (settingManager.hideTabX ? 0 : static_cast<int>(16) * scale);
    QRect rect(0, 0, this->width() - rectW, this->height());

    // draw text
    painter.drawText(rect, this->getTitle(), QTextOption(Qt::AlignCenter));

    // draw close x
    if (!settingManager.hideTabX && (mouseOver || selected)) {
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
        if (!singletons::SettingManager::getInstance().hideTabX && this->mouseDownX &&
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
    if (!singletons::SettingManager::getInstance().hideTabX) {
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

}  // namespace widgets
}  // namespace chatterino
