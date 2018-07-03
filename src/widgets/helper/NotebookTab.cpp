#include "widgets/helper/NotebookTab.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "debug/Log.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/Clamp.hpp"
#include "util/Helpers.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/dialogs/TextInputDialog.hpp"

#include <QApplication>
#include <QDebug>
#include <QLinearGradient>
#include <QMimeData>
#include <QPainter>
#include <boost/bind.hpp>

namespace chatterino {

NotebookTab::NotebookTab(Notebook *notebook)
    : RippleEffectButton(notebook)
    , positionChangedAnimation_(this, "pos")
    , notebook_(notebook)
    , menu_(this)
{
    auto app = getApp();

    this->setAcceptDrops(true);

    this->positionChangedAnimation_.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));

    app->settings->showTabCloseButton.connect(boost::bind(&NotebookTab::hideTabXChanged, this, _1),
                                              this->managedConnections_);

    this->setMouseTracking(true);

    this->menu_.addAction("Rename", [this]() {
        TextInputDialog d(this);

        d.setWindowTitle("Change tab title (Leave empty for default behaviour)");
        d.setText(this->getCustomTitle());
        d.highlightText();

        if (d.exec() == QDialog::Accepted) {
            QString newTitle = d.getText();
            this->setCustomTitle(newTitle);
        }
    });

    //    QAction *enableHighlightsOnNewMessageAction =
    //        new QAction("Enable highlights on new message", &this->menu);
    //    enableHighlightsOnNewMessageAction->setCheckable(true);

    this->menu_.addAction("Close", [=]() { this->notebook_->removePage(this->page); });

    //    this->menu.addAction(enableHighlightsOnNewMessageAction);

    //    QObject::connect(enableHighlightsOnNewMessageAction, &QAction::toggled, [this](bool
    //    newValue) {
    //        Log("New value is {}", newValue);  //
    //    });
}

void NotebookTab::themeRefreshEvent()
{
    this->update();

    //    this->setMouseEffectColor(QColor("#999"));
    this->setMouseEffectColor(this->themeManager->tabs.regular.text);
}

void NotebookTab::updateSize()
{
    float scale = getScale();

    int width;
    QFontMetrics metrics = getApp()->fonts->getFontMetrics(
        FontStyle::UiTabs, float(qreal(this->getScale()) * this->devicePixelRatioF()));

    if (this->hasXButton()) {
        width = (metrics.width(this->getTitle()) + int(32 * scale));
    } else {
        width = (metrics.width(this->getTitle()) + int(16 * scale));
    }

    width = clamp(width, this->height(), int(150 * scale));

    if (this->width() != width) {
        this->resize(width, int(NOTEBOOK_TAB_HEIGHT * scale));
        this->notebook_->performLayout();
    }
}

const QString &NotebookTab::getCustomTitle() const
{
    return this->customTitle_;
}

void NotebookTab::setCustomTitle(const QString &newTitle)
{
    if (this->customTitle_ != newTitle) {
        this->customTitle_ = newTitle;
        this->titleUpdated();
    }
}

void NotebookTab::resetCustomTitle()
{
    this->setCustomTitle(QString());
}

bool NotebookTab::hasCustomTitle() const
{
    return !this->customTitle_.isEmpty();
}

void NotebookTab::setDefaultTitle(const QString &title)
{
    if (this->defaultTitle_ != title) {
        this->defaultTitle_ = title;

        if (this->customTitle_.isEmpty()) {
            this->titleUpdated();
        }
    }
}

const QString &NotebookTab::getDefaultTitle() const
{
    return this->defaultTitle_;
}

const QString &NotebookTab::getTitle() const
{
    return this->customTitle_.isEmpty() ? this->defaultTitle_ : this->customTitle_;
}

void NotebookTab::titleUpdated()
{
    this->updateSize();
    this->update();
}

bool NotebookTab::isSelected() const
{
    return this->selected_;
}

void NotebookTab::setSelected(bool value)
{
    this->selected_ = value;

    this->highlightState_ = HighlightState::None;

    this->update();
}

void NotebookTab::setHighlightState(HighlightState newHighlightStyle)
{
    if (this->isSelected()) {
        return;
    }

    if (this->highlightState_ != HighlightState::Highlighted) {
        this->highlightState_ = newHighlightStyle;

        this->update();
    }
}

QRect NotebookTab::getDesiredRect() const
{
    return QRect(this->positionAnimationDesiredPoint_, size());
}

void NotebookTab::hideTabXChanged(bool)
{
    this->updateSize();
    this->update();
}

void NotebookTab::moveAnimated(QPoint pos, bool animated)
{
    this->positionAnimationDesiredPoint_ = pos;

    QWidget *w = this->window();

    if ((w != nullptr && !w->isVisible()) || !animated || !this->positionChangedAnimationRunning_) {
        this->move(pos);

        this->positionChangedAnimationRunning_ = true;
        return;
    }

    if (this->positionChangedAnimation_.endValue() == pos) {
        return;
    }

    this->positionChangedAnimation_.stop();
    this->positionChangedAnimation_.setDuration(75);
    this->positionChangedAnimation_.setStartValue(this->pos());
    this->positionChangedAnimation_.setEndValue(pos);
    this->positionChangedAnimation_.start();
}

void NotebookTab::paintEvent(QPaintEvent *)
{
    auto app = getApp();
    QPainter painter(this);
    float scale = this->getScale();

    painter.setFont(getApp()->fonts->getFont(
        FontStyle::UiTabs, scale * 96.f / this->logicalDpiX() * this->devicePixelRatioF()));
    QFontMetrics metrics = app->fonts->getFontMetrics(
        FontStyle::UiTabs, scale * 96.f / this->logicalDpiX() * this->devicePixelRatioF());

    int height = int(scale * NOTEBOOK_TAB_HEIGHT);
    //    int fullHeight = (int)(scale * 48);

    // select the right tab colors
    Theme::TabColors colors;
    Theme::TabColors regular = this->themeManager->tabs.regular;

    if (this->selected_) {
        colors = this->themeManager->tabs.selected;
    } else if (this->highlightState_ == HighlightState::Highlighted) {
        colors = this->themeManager->tabs.highlighted;
    } else if (this->highlightState_ == HighlightState::NewMessage) {
        colors = this->themeManager->tabs.newMessage;
    } else {
        colors = this->themeManager->tabs.regular;
    }

    bool windowFocused = this->window() == QApplication::activeWindow();
    // || SettingsDialog::getHandle() == QApplication::activeWindow();

    QBrush tabBackground = /*this->mouseOver_ ? colors.backgrounds.hover
                                            :*/
        (windowFocused ? colors.backgrounds.regular : colors.backgrounds.unfocused);

    //    painter.fillRect(rect(), this->mouseOver_ ? regular.backgrounds.hover
    //                                              : (windowFocused ? regular.backgrounds.regular
    //                                                               :
    //                                                               regular.backgrounds.unfocused));

    // fill the tab background
    auto bgRect = rect();
    bgRect.setTop(bgRect.top() + 2);

    painter.fillRect(bgRect, tabBackground);

    // draw border
    //    painter.setPen(QPen("#fff"));
    //    QPainterPath path(QPointF(0, height));
    //    path.lineTo(0, 0);
    //    path.lineTo(this->width() - 1, 0);
    //    path.lineTo(this->width() - 1, this->height() - 1);
    //    path.lineTo(0, this->height() - 1);
    //    painter.drawPath(path);

    // top line
    painter.fillRect(QRectF(0, (this->selected_ ? 0.f : 1.f) * scale, this->width(),
                            (this->selected_ ? 2.f : 1.f) * scale),
                     this->mouseOver_
                         ? colors.line.hover
                         : (windowFocused ? colors.line.regular : colors.line.unfocused));

    // set the pen color
    painter.setPen(colors.text);

    // set area for text
    int rectW = (!app->settings->showTabCloseButton ? 0 : int(16 * scale));
    QRect rect(0, 0, this->width() - rectW, height);

    // draw text
    int offset = int(scale * 8);
    QRect textRect(offset, this->selected_ ? 1 : 2, this->width() - offset - offset, height);

    if (this->shouldDrawXButton()) {
        textRect.setRight(textRect.right() - this->height() / 2);
    }

    int width = metrics.width(this->getTitle());
    Qt::Alignment alignment = width > textRect.width() ? Qt::AlignLeft | Qt::AlignVCenter
                                                       : Qt::AlignHCenter | Qt::AlignVCenter;

    QTextOption option(alignment);
    option.setWrapMode(QTextOption::NoWrap);
    painter.drawText(textRect, this->getTitle(), option);

    // draw close x
    if (this->shouldDrawXButton()) {
        QRect xRect = this->getXRect();
        if (!xRect.isNull()) {
            painter.setBrush(QColor("#fff"));

            if (this->mouseOverX_) {
                painter.fillRect(xRect, QColor(0, 0, 0, 64));

                if (this->mouseDownX_) {
                    painter.fillRect(xRect, QColor(0, 0, 0, 64));
                }
            }

            int a = static_cast<int>(scale * 4);

            painter.drawLine(xRect.topLeft() + QPoint(a, a), xRect.bottomRight() + QPoint(-a, -a));
            painter.drawLine(xRect.topRight() + QPoint(-a, a), xRect.bottomLeft() + QPoint(a, -a));
        }
    }

    // draw line at bottom
    if (!this->selected_) {
        painter.fillRect(0, this->height() - 1, this->width(), 1, app->themes->window.background);

        this->fancyPaint(painter);
    }
}

bool NotebookTab::hasXButton()
{
    return getApp()->settings->showTabCloseButton && this->notebook_->getAllowUserTabManagement();
}

bool NotebookTab::shouldDrawXButton()
{
    return this->hasXButton() && (this->mouseOver_ || this->selected_);
}

void NotebookTab::mousePressEvent(QMouseEvent *event)
{
    this->mouseDown_ = true;
    this->mouseDownX_ = this->getXRect().contains(event->pos());

    this->update();

    this->notebook_->select(page);

    if (this->notebook_->getAllowUserTabManagement()) {
        switch (event->button()) {
            case Qt::RightButton: {
                this->menu_.popup(event->globalPos());
            } break;
            default:;
        }
    }
}

void NotebookTab::mouseReleaseEvent(QMouseEvent *event)
{
    this->mouseDown_ = false;

    auto removeThisPage = [this] {
        auto reply = QMessageBox::question(this, "Remove this tab",
                                           "Are you sure that you want to remove this tab?",
                                           QMessageBox::Yes | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            this->notebook_->removePage(this->page);
        }
    };

    if (event->button() == Qt::MiddleButton) {
        if (this->rect().contains(event->pos())) {
            removeThisPage();
        }
    } else {
        if (this->hasXButton() && this->mouseDownX_ && this->getXRect().contains(event->pos())) {
            this->mouseDownX_ = false;

            removeThisPage();
        } else {
            this->update();
        }
    }
}

void NotebookTab::enterEvent(QEvent *event)
{
    this->mouseOver_ = true;

    this->update();

    RippleEffectButton::enterEvent(event);
}

void NotebookTab::leaveEvent(QEvent *event)
{
    this->mouseOverX_ = false;
    this->mouseOver_ = false;

    this->update();

    RippleEffectButton::leaveEvent(event);
}

void NotebookTab::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
        return;

    if (!SplitContainer::isDraggingSplit)
        return;

    if (this->notebook_->getAllowUserTabManagement()) {
        this->notebook_->select(this->page);
    }
}

void NotebookTab::mouseMoveEvent(QMouseEvent *event)
{
    auto app = getApp();

    if (app->settings->showTabCloseButton && this->notebook_->getAllowUserTabManagement())  //
    {
        bool overX = this->getXRect().contains(event->pos());

        if (overX != this->mouseOverX_) {
            // Over X state has been changed (we either left or entered it;
            this->mouseOverX_ = overX;

            this->update();
        }
    }

    QPoint relPoint = this->mapToParent(event->pos());

    if (this->mouseDown_ && !this->getDesiredRect().contains(relPoint) &&
        this->notebook_->getAllowUserTabManagement())  //
    {
        int index;
        QWidget *clickedPage = this->notebook_->tabAt(relPoint, index, this->width());

        if (clickedPage != nullptr && clickedPage != this->page) {
            this->notebook_->rearrangePage(this->page, index);
        }
    }

    RippleEffectButton::mouseMoveEvent(event);
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

}  // namespace chatterino
