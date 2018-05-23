#include "widgets/helper/notebooktab.hpp"

#include "application.hpp"
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
#include <QLinearGradient>
#include <QPainter>
#include <boost/bind.hpp>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *_notebook)
    : BaseWidget(_notebook)
    , positionChangedAnimation(this, "pos")
    , notebook(_notebook)
    , menu(this)
{
    auto app = getApp();

    this->setAcceptDrops(true);

    this->positionChangedAnimation.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));

    app->settings->showTabCloseButton.connect(boost::bind(&NotebookTab::hideTabXChanged, this, _1),
                                              this->managedConnections);

    this->setMouseTracking(true);

    this->menu.addAction("Rename", [this]() {
        TextInputDialog d(this);

        d.setWindowTitle("Change tab title (Leave empty for default behaviour)");
        if (this->useDefaultTitle) {
            d.setText("");
        } else {
            d.setText(this->getTitle());
            d.highlightText();
        }

        if (d.exec() == QDialog::Accepted) {
            QString newTitle = d.getText();
            if (newTitle.isEmpty()) {
                this->useDefaultTitle = true;

                // fourtf: xD
                //                this->page->refreshTitle();
            } else {
                this->useDefaultTitle = false;
                this->setTitle(newTitle);
            }
        }
    });

    //    QAction *enableHighlightsOnNewMessageAction =
    //        new QAction("Enable highlights on new message", &this->menu);
    //    enableHighlightsOnNewMessageAction->setCheckable(true);

    this->menu.addAction("Close", [=]() { this->notebook->removePage(this->page); });

    //    this->menu.addAction(enableHighlightsOnNewMessageAction);

    //    QObject::connect(enableHighlightsOnNewMessageAction, &QAction::toggled, [this](bool
    //    newValue) {
    //        debug::Log("New value is {}", newValue);  //
    //    });
}

void NotebookTab::themeRefreshEvent()
{
    this->update();
}

void NotebookTab::updateSize()
{
    auto app = getApp();
    float scale = getScale();

    int width;
    QFontMetrics metrics = getApp()->fonts->getFontMetrics(FontStyle::UiTabs, this->getScale());

    if (this->hasXButton()) {
        width = (int)((metrics.width(this->title) + 32) * scale);
    } else {
        width = (int)((metrics.width(this->title) + 16) * scale);
    }

    width = std::min((int)(150 * scale), width);

    if (this->width() != width) {
        this->resize(width, (int)(NOTEBOOK_TAB_HEIGHT * scale));
        this->notebook->performLayout();
    }

    //    if (this->parent() != nullptr) {
    //        (static_cast<Notebook2 *>(this->parent()))->performLayout(true);
    //    }
}

const QString &NotebookTab::getTitle() const
{
    return this->title;
}

void NotebookTab::setTitle(const QString &newTitle)
{
    if (this->title != newTitle) {
        this->title = newTitle;
        this->updateSize();
        this->update();
    }
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

    if (this->highlightState != HighlightState::Highlighted) {
        this->highlightState = newHighlightStyle;

        this->update();
    }
}

QRect NotebookTab::getDesiredRect() const
{
    return QRect(positionAnimationDesiredPoint, size());
}

void NotebookTab::hideTabXChanged(bool)
{
    this->updateSize();
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
    auto app = getApp();
    QPainter painter(this);
    float scale = this->getScale();

    painter.setFont(getApp()->fonts->getFont(FontStyle::UiTabs, scale));

    int height = (int)(scale * NOTEBOOK_TAB_HEIGHT);
    //    int fullHeight = (int)(scale * 48);

    // select the right tab colors
    singletons::ThemeManager::TabColors colors;
    singletons::ThemeManager::TabColors regular = this->themeManager->tabs.regular;

    if (this->selected) {
        colors = this->themeManager->tabs.selected;
    } else if (this->highlightState == HighlightState::Highlighted) {
        colors = this->themeManager->tabs.highlighted;
    } else if (this->highlightState == HighlightState::NewMessage) {
        colors = this->themeManager->tabs.newMessage;
    } else {
        colors = this->themeManager->tabs.regular;
    }

    bool windowFocused = this->window() == QApplication::activeWindow();
    // || SettingsDialog::getHandle() == QApplication::activeWindow();

    QBrush tabBackground = this->mouseOver ? colors.backgrounds.hover
                                           : (windowFocused ? colors.backgrounds.regular
                                                            : colors.backgrounds.unfocused);

    painter.fillRect(rect(), this->mouseOver ? regular.backgrounds.hover
                                             : (windowFocused ? regular.backgrounds.regular
                                                              : regular.backgrounds.unfocused));

    // fill the tab background
    painter.fillRect(rect(), tabBackground);

    // draw border
    //    painter.setPen(QPen("#fff"));
    //    QPainterPath path(QPointF(0, height));
    //    path.lineTo(0, 0);
    //    path.lineTo(this->width() - 1, 0);
    //    path.lineTo(this->width() - 1, this->height() - 1);
    //    path.lineTo(0, this->height() - 1);
    //    painter.drawPath(path);

    // top line
    painter.fillRect(QRectF(0, (this->selected ? 0.f : 1.f) * scale, this->width(),
                            (this->selected ? 2.f : 1.f) * scale),
                     this->mouseOver
                         ? colors.line.hover
                         : (windowFocused ? colors.line.regular : colors.line.unfocused));

    // set the pen color
    painter.setPen(colors.text);

    // set area for text
    int rectW = (!app->settings->showTabCloseButton ? 0 : static_cast<int>(16) * scale);
    QRect rect(0, 0, this->width() - rectW, height);

    // draw text
    if (true) {  // legacy
        //    painter.drawText(rect, this->getTitle(), QTextOption(Qt::AlignCenter));
        int offset = (int)(scale * 8);
        QRect textRect(offset, this->selected ? 0 : 1, this->width() - offset - offset, height);

        if (this->shouldDrawXButton()) {
            textRect.setRight(textRect.right() - this->height() / 2);
        }

        QTextOption option(Qt::AlignLeft | Qt::AlignVCenter);
        option.setWrapMode(QTextOption::NoWrap);
        painter.drawText(textRect, this->getTitle(), option);
    } else {
        //    QTextOption option(Qt::AlignLeft | Qt::AlignVCenter);
        //    option.setWrapMode(QTextOption::NoWrap);
        //    int offset = (int)(scale * 16);
        //    QRect textRect(offset, 0, this->width() - offset - offset, height);
        //    painter.drawText(textRect, this->getTitle(), option);
    }

    // draw close x
    if (this->shouldDrawXButton()) {
        QRect xRect = this->getXRect();
        if (!xRect.isNull()) {
            painter.setBrush(QColor("#fff"));

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

    // draw line at bottom
    if (!this->selected) {
        painter.fillRect(0, this->height() - 1, this->width(), 1, app->themes->window.background);
    }
}

bool NotebookTab::hasXButton()
{
    return getApp()->settings->showTabCloseButton && this->notebook->getAllowUserTabManagement();
}

bool NotebookTab::shouldDrawXButton()
{
    return this->hasXButton() && (mouseOver || selected);
}

void NotebookTab::mousePressEvent(QMouseEvent *event)
{
    this->mouseDown = true;
    this->mouseDownX = this->getXRect().contains(event->pos());

    this->update();

    this->notebook->select(page);

    if (this->notebook->getAllowUserTabManagement()) {
        switch (event->button()) {
            case Qt::RightButton: {
                this->menu.popup(event->globalPos());
            } break;
        }
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
        if (this->hasXButton() && this->mouseDownX && this->getXRect().contains(event->pos())) {
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
    if (this->notebook->getAllowUserTabManagement()) {
        this->notebook->select(this->page);
    }
}

void NotebookTab::mouseMoveEvent(QMouseEvent *event)
{
    auto app = getApp();

    if (app->settings->showTabCloseButton && this->notebook->getAllowUserTabManagement())  //
    {
        bool overX = this->getXRect().contains(event->pos());

        if (overX != this->mouseOverX) {
            // Over X state has been changed (we either left or entered it;
            this->mouseOverX = overX;

            this->update();
        }
    }

    QPoint relPoint = this->mapToParent(event->pos());

    if (this->mouseDown && !this->getDesiredRect().contains(relPoint) &&
        this->notebook->getAllowUserTabManagement())  //
    {
        int index;
        QWidget *clickedPage = notebook->tabAt(relPoint, index, this->width());

        //        assert(clickedPage);

        if (clickedPage != nullptr && clickedPage != this->page) {
            this->notebook->rearrangePage(this->page, index);
        }
    }
}

QRect NotebookTab::getXRect()
{
    //    if (!this->notebook->getAllowUserTabManagement()) {
    //        return QRect();
    //    }

    float s = this->getScale();
    return QRect(this->width() - static_cast<int>(20 * s), static_cast<int>(6 * s),
                 static_cast<int>(16 * s), static_cast<int>(16 * s));
}

}  // namespace widgets
}  // namespace chatterino
