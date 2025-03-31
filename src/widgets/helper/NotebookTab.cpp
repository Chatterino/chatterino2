#include "widgets/helper/NotebookTab.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "controllers/hotkeys/HotkeyCategory.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/DraggedSplit.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <boost/bind/bind.hpp>
#include <boost/container_hash/hash.hpp>
#include <QAbstractAnimation>
#include <QApplication>
#include <QDebug>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLinearGradient>
#include <QLineEdit>
#include <QMimeData>
#include <QPainter>

#include <algorithm>

namespace chatterino {
namespace {
    // Translates the given rectangle by an amount in the direction to appear like the tab is selected.
    // For example, if location is Top, the rectangle will be translated in the negative Y direction,
    // or "up" on the screen, by amount.
    void translateRectForLocation(QRect &rect, NotebookTabLocation location,
                                  int amount)
    {
        switch (location)
        {
            case NotebookTabLocation::Top:
                rect.translate(0, -amount);
                break;
            case NotebookTabLocation::Left:
                rect.translate(-amount, 0);
                break;
            case NotebookTabLocation::Right:
                rect.translate(amount, 0);
                break;
            case NotebookTabLocation::Bottom:
                rect.translate(0, amount);
                break;
        }
    }
}  // namespace

NotebookTab::NotebookTab(Notebook *notebook)
    : Button(notebook)
    , positionChangedAnimation_(this, "pos")
    , notebook_(notebook)
    , menu_(this)
{
    this->setAcceptDrops(true);

    this->positionChangedAnimation_.setEasingCurve(
        QEasingCurve(QEasingCurve::InCubic));

    getSettings()->showTabCloseButton.connectSimple(
        boost::bind(&NotebookTab::hideTabXChanged, this),
        this->managedConnections_);
    getSettings()->showTabLive.connect(
        [this](auto, auto) {
            this->update();
        },
        this->managedConnections_);

    this->setMouseTracking(true);

    this->menu_.addAction("Rename Tab", [this]() {
        this->showRenameDialog();
    });

    // XXX: this doesn't update after changing hotkeys

    this->menu_.addAction(
        "Close Tab",
        [this]() {
            this->notebook_->removePage(this->page);
        },
        getApp()->getHotkeys()->getDisplaySequence(HotkeyCategory::Window,
                                                   "removeTab"));

    this->menu_.addAction(
        "Popup Tab",
        [this]() {
            if (auto *container = dynamic_cast<SplitContainer *>(this->page))
            {
                container->popup();
            }
        },
        getApp()->getHotkeys()->getDisplaySequence(HotkeyCategory::Window,
                                                   "popup", {{"window"}}));

    this->menu_.addAction("Duplicate Tab", [this]() {
        this->notebook_->duplicatePage(this->page);
    });

    highlightNewMessagesAction_ =
        new QAction("Mark Tab as Unread on New Messages", &this->menu_);
    highlightNewMessagesAction_->setCheckable(true);
    highlightNewMessagesAction_->setChecked(highlightEnabled_);
    QObject::connect(highlightNewMessagesAction_, &QAction::triggered,
                     [this](bool checked) {
                         this->highlightEnabled_ = checked;
                     });
    this->menu_.addAction(highlightNewMessagesAction_);

    this->menu_.addSeparator();

    this->notebook_->addNotebookActionsToMenu(&this->menu_);
}

void NotebookTab::showRenameDialog()
{
    auto *dialog = new QDialog(this);

    auto *vbox = new QVBoxLayout;

    auto *lineEdit = new QLineEdit;
    lineEdit->setText(this->getCustomTitle());
    lineEdit->setPlaceholderText(this->getDefaultTitle());
    lineEdit->selectAll();

    vbox->addWidget(new QLabel("Name:"));
    vbox->addWidget(lineEdit);
    vbox->addStretch(1);

    auto *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    vbox->addWidget(buttonBox);
    dialog->setLayout(vbox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [dialog] {
        dialog->accept();
        dialog->close();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, [dialog] {
        dialog->reject();
        dialog->close();
    });

    dialog->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    dialog->setMinimumSize(dialog->minimumSizeHint().width() + 50,
                           dialog->minimumSizeHint().height() + 10);

    dialog->setWindowFlags(
        (dialog->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    dialog->setWindowTitle("Rename Tab");

    if (dialog->exec() == QDialog::Accepted)
    {
        QString newTitle = lineEdit->text();
        this->setCustomTitle(newTitle);
    }
}

void NotebookTab::themeChangedEvent()
{
    this->update();

    //    this->setMouseEffectColor(QColor("#999"));
    this->setMouseEffectColor(this->theme->tabs.regular.text);
}

void NotebookTab::growWidth(int width)
{
    if (this->growWidth_ != width)
    {
        this->growWidth_ = width;
        this->updateSize();
    }
    else
    {
        this->growWidth_ = width;
    }
}

int NotebookTab::normalTabWidth() const
{
    return this->normalTabWidthForHeight(this->height());
}

int NotebookTab::normalTabWidthForHeight(int height) const
{
    float scale = this->scale();
    int width = 0;

    QFontMetrics metrics =
        getApp()->getFonts()->getFontMetrics(FontStyle::UiTabs, scale);

    if (this->hasXButton())
    {
        width = (metrics.horizontalAdvance(this->getTitle()) + int(32 * scale));
    }
    else
    {
        width = (metrics.horizontalAdvance(this->getTitle()) + int(16 * scale));
    }

    if (static_cast<float>(height) > 150 * scale)
    {
        width = height;
    }
    else
    {
        width = std::clamp(width, height, static_cast<int>(150 * scale));
    }

    return width;
}

void NotebookTab::updateSize()
{
    float scale = this->scale();
    auto height = static_cast<int>(NOTEBOOK_TAB_HEIGHT * scale);
    int width = this->normalTabWidthForHeight(height);

    if (width < this->growWidth_)
    {
        width = this->growWidth_;
    }

    if (this->width() != width || this->height() != height)
    {
        this->resize(width, height);
        this->notebook_->refresh();
    }
}

const QString &NotebookTab::getCustomTitle() const
{
    return this->customTitle_;
}

void NotebookTab::setCustomTitle(const QString &newTitle)
{
    if (this->customTitle_ != newTitle)
    {
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
    if (this->defaultTitle_ != title)
    {
        this->defaultTitle_ = title;

        if (this->customTitle_.isEmpty())
        {
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
    return this->customTitle_.isEmpty() ? this->defaultTitle_
                                        : this->customTitle_;
}

void NotebookTab::titleUpdated()
{
    // Queue up save because: Tab title changed
    getApp()->getWindows()->queueSave();
    this->notebook_->refresh();
    this->updateSize();
    this->update();
}

bool NotebookTab::isSelected() const
{
    return this->selected_;
}

void NotebookTab::removeHighlightStateChangeSources(
    const HighlightSources &toRemove)
{
    for (const auto &[source, _] : toRemove)
    {
        this->removeHighlightSource(source);
    }
}

void NotebookTab::removeHighlightSource(
    const ChannelView::ChannelViewID &source)
{
    this->highlightSources_.erase(source);
}

void NotebookTab::newHighlightSourceAdded(const ChannelView &channelViewSource)
{
    auto channelViewId = channelViewSource.getID();
    this->removeHighlightSource(channelViewId);
    this->updateHighlightStateDueSourcesChange();

    auto *splitNotebook = dynamic_cast<SplitNotebook *>(this->notebook_);
    if (splitNotebook)
    {
        for (int i = 0; i < splitNotebook->getPageCount(); ++i)
        {
            auto *splitContainer =
                dynamic_cast<SplitContainer *>(splitNotebook->getPageAt(i));
            if (splitContainer)
            {
                auto *tab = splitContainer->getTab();
                if (tab && tab != this)
                {
                    tab->removeHighlightSource(channelViewId);
                    tab->updateHighlightStateDueSourcesChange();
                }
            }
        }
    }
}

void NotebookTab::updateHighlightStateDueSourcesChange()
{
    if (std::ranges::any_of(this->highlightSources_, [](const auto &keyval) {
            return keyval.second == HighlightState::Highlighted;
        }))
    {
        assert(this->highlightState_ == HighlightState::Highlighted);
        return;
    }

    if (std::ranges::any_of(this->highlightSources_, [](const auto &keyval) {
            return keyval.second == HighlightState::NewMessage;
        }))
    {
        if (this->highlightState_ != HighlightState::NewMessage)
        {
            this->highlightState_ = HighlightState::NewMessage;
            this->update();
        }
    }
    else
    {
        if (this->highlightState_ != HighlightState::None)
        {
            this->highlightState_ = HighlightState::None;
            this->update();
        }
    }

    assert(this->highlightState_ != HighlightState::Highlighted);
}

void NotebookTab::copyHighlightStateAndSourcesFrom(const NotebookTab *sourceTab)
{
    if (this->isSelected())
    {
        assert(this->highlightSources_.empty());
        assert(this->highlightState_ == HighlightState::None);
        return;
    }

    this->highlightSources_ = sourceTab->highlightSources_;

    if (!this->highlightEnabled_ &&
        sourceTab->highlightState_ == HighlightState::NewMessage)
    {
        return;
    }

    if (this->highlightState_ == sourceTab->highlightState_ ||
        this->highlightState_ == HighlightState::Highlighted)
    {
        return;
    }

    this->highlightState_ = sourceTab->highlightState_;
    this->update();
}

void NotebookTab::setSelected(bool value)
{
    this->selected_ = value;

    if (value)
    {
        auto *splitNotebook = dynamic_cast<SplitNotebook *>(this->notebook_);
        if (splitNotebook)
        {
            for (int i = 0; i < splitNotebook->getPageCount(); ++i)
            {
                auto *splitContainer =
                    dynamic_cast<SplitContainer *>(splitNotebook->getPageAt(i));
                if (splitContainer)
                {
                    auto *tab = splitContainer->getTab();
                    if (tab && tab != this)
                    {
                        tab->removeHighlightStateChangeSources(
                            this->highlightSources_);
                        tab->updateHighlightStateDueSourcesChange();
                    }
                }
            }
        }
    }

    this->highlightSources_.clear();
    this->highlightState_ = HighlightState::None;

    this->update();
}

void NotebookTab::setInLastRow(bool value)
{
    if (this->isInLastRow_ != value)
    {
        this->isInLastRow_ = value;
        this->update();
    }
}

void NotebookTab::setTabLocation(NotebookTabLocation location)
{
    if (this->tabLocation_ != location)
    {
        this->tabLocation_ = location;
        this->update();
    }
}

bool NotebookTab::setRerun(bool isRerun)
{
    if (this->isRerun_ != isRerun)
    {
        this->isRerun_ = isRerun;
        this->update();
        return true;
    }

    return false;
}

bool NotebookTab::setLive(bool isLive)
{
    if (this->isLive_ != isLive)
    {
        this->isLive_ = isLive;
        this->update();
        return true;
    }

    return false;
}

bool NotebookTab::isLive() const
{
    return this->isLive_;
}

HighlightState NotebookTab::highlightState() const
{
    return this->highlightState_;
}

void NotebookTab::setHighlightState(HighlightState newHighlightStyle)
{
    if (this->isSelected())
    {
        assert(this->highlightSources_.empty());
        assert(this->highlightState_ == HighlightState::None);
        return;
    }

    this->highlightSources_.clear();

    if (!this->highlightEnabled_ &&
        newHighlightStyle == HighlightState::NewMessage)
    {
        return;
    }

    if (this->highlightState_ == newHighlightStyle ||
        this->highlightState_ == HighlightState::Highlighted)
    {
        return;
    }

    this->highlightState_ = newHighlightStyle;
    this->update();
}

void NotebookTab::updateHighlightState(HighlightState newHighlightStyle,
                                       const ChannelView &channelViewSource)
{
    if (this->isSelected())
    {
        assert(this->highlightSources_.empty());
        assert(this->highlightState_ == HighlightState::None);
        return;
    }

    if (!this->shouldMessageHighlight(channelViewSource))
    {
        return;
    }

    if (!this->highlightEnabled_ &&
        newHighlightStyle == HighlightState::NewMessage)
    {
        return;
    }

    // message is highlighting unvisible tab

    auto channelViewId = channelViewSource.getID();

    switch (newHighlightStyle)
    {
        case HighlightState::Highlighted:
            // override lower states
            this->highlightSources_.insert_or_assign(channelViewId,
                                                     newHighlightStyle);
        case HighlightState::NewMessage: {
            // only insert if no state already there to avoid overriding
            if (!this->highlightSources_.contains(channelViewId))
            {
                this->highlightSources_.emplace(channelViewId,
                                                newHighlightStyle);
            }
            break;
        }
        case HighlightState::None:
            break;
    }

    if (this->highlightState_ == newHighlightStyle ||
        this->highlightState_ == HighlightState::Highlighted)
    {
        return;
    }

    this->highlightState_ = newHighlightStyle;
    this->update();
}

bool NotebookTab::shouldMessageHighlight(
    const ChannelView &channelViewSource) const
{
    auto *visibleSplitContainer =
        dynamic_cast<SplitContainer *>(this->notebook_->getSelectedPage());
    if (visibleSplitContainer != nullptr)
    {
        const auto &visibleSplits = visibleSplitContainer->getSplits();
        for (const auto &visibleSplit : visibleSplits)
        {
            if (channelViewSource.getID() ==
                visibleSplit->getChannelView().getID())
            {
                return false;
            }
        }
    }

    return true;
}

void NotebookTab::setHighlightsEnabled(const bool &newVal)
{
    this->highlightNewMessagesAction_->setChecked(newVal);
    this->highlightEnabled_ = newVal;
}

bool NotebookTab::hasHighlightsEnabled() const
{
    return this->highlightEnabled_;
}

QRect NotebookTab::getDesiredRect() const
{
    return QRect(this->positionAnimationDesiredPoint_, size());
}

void NotebookTab::hideTabXChanged()
{
    this->updateSize();
    this->update();
}

void NotebookTab::moveAnimated(QPoint targetPos, bool animated)
{
    this->positionAnimationDesiredPoint_ = targetPos;

    if (!animated || !this->notebook_->isVisible())
    {
        this->move(targetPos);
        return;
    }

    if (this->positionChangedAnimation_.state() ==
            QAbstractAnimation::Running &&
        this->positionChangedAnimation_.endValue() == targetPos)
    {
        return;
    }

    this->positionChangedAnimation_.stop();
    this->positionChangedAnimation_.setDuration(75);
    this->positionChangedAnimation_.setStartValue(this->pos());
    this->positionChangedAnimation_.setEndValue(targetPos);
    this->positionChangedAnimation_.start();
}

void NotebookTab::paintEvent(QPaintEvent *)
{
    auto *app = getApp();
    QPainter painter(this);
    float scale = this->scale();

    painter.setFont(app->getFonts()->getFont(FontStyle::UiTabs, scale));
    QFontMetrics metrics =
        app->getFonts()->getFontMetrics(FontStyle::UiTabs, scale);

    int height = int(scale * NOTEBOOK_TAB_HEIGHT);

    // select the right tab colors
    Theme::TabColors colors;

    if (this->selected_)
    {
        colors = this->theme->tabs.selected;
    }
    else if (this->highlightState_ == HighlightState::Highlighted)
    {
        colors = this->theme->tabs.highlighted;
    }
    else if (this->highlightState_ == HighlightState::NewMessage)
    {
        colors = this->theme->tabs.newMessage;
    }
    else
    {
        colors = this->theme->tabs.regular;
    }

    bool windowFocused = this->window() == QApplication::activeWindow();

    QBrush tabBackground = /*this->mouseOver_ ? colors.backgrounds.hover
                                 :*/
        (windowFocused ? colors.backgrounds.regular
                       : colors.backgrounds.unfocused);

    auto selectionOffset = ceil((this->selected_ ? 0.f : 1.f) * scale);

    // fill the tab background
    auto bgRect = this->rect();
    switch (this->tabLocation_)
    {
        case NotebookTabLocation::Top:
            bgRect.setTop(selectionOffset);
            break;
        case NotebookTabLocation::Left:
            bgRect.setLeft(selectionOffset);
            break;
        case NotebookTabLocation::Right:
            bgRect.setRight(bgRect.width() - selectionOffset);
            break;
        case NotebookTabLocation::Bottom:
            bgRect.setBottom(bgRect.height() - selectionOffset);
            break;
    }

    painter.fillRect(bgRect, tabBackground);

    // draw color indicator line
    auto lineThickness = ceil((this->selected_ ? 2.f : 1.f) * scale);
    auto lineColor = this->mouseOver_ ? colors.line.hover
                                      : (windowFocused ? colors.line.regular
                                                       : colors.line.unfocused);

    QRect lineRect;
    switch (this->tabLocation_)
    {
        case NotebookTabLocation::Top:
            lineRect =
                QRect(bgRect.left(), bgRect.y(), bgRect.width(), lineThickness);
            break;
        case NotebookTabLocation::Left:
            lineRect =
                QRect(bgRect.x(), bgRect.top(), lineThickness, bgRect.height());
            break;
        case NotebookTabLocation::Right:
            lineRect = QRect(bgRect.right() - lineThickness, bgRect.top(),
                             lineThickness, bgRect.height());
            break;
        case NotebookTabLocation::Bottom:
            lineRect = QRect(bgRect.left(), bgRect.bottom() - lineThickness,
                             bgRect.width(), lineThickness);
            break;
    }

    painter.fillRect(lineRect, lineColor);

    // draw live indicator
    if ((this->isLive_ || this->isRerun_) && getSettings()->showTabLive)
    {
        // Live overrides rerun
        QBrush b;
        if (this->isLive_)
        {
            painter.setPen(this->theme->tabs.liveIndicator);
            b.setColor(this->theme->tabs.liveIndicator);
        }
        else
        {
            painter.setPen(this->theme->tabs.rerunIndicator);
            b.setColor(this->theme->tabs.rerunIndicator);
        }

        painter.setRenderHint(QPainter::Antialiasing);
        b.setStyle(Qt::SolidPattern);
        painter.setBrush(b);

        auto x = this->width() - (7 * scale);
        auto y = 4 * scale;
        auto diameter = 4 * scale;
        QRect liveIndicatorRect(x, y, diameter, diameter);
        translateRectForLocation(liveIndicatorRect, this->tabLocation_,
                                 this->selected_ ? 0 : -1);
        painter.drawEllipse(liveIndicatorRect);
    }

    // set the pen color
    painter.setPen(colors.text);

    // set area for text
    int rectW = (!getSettings()->showTabCloseButton ? 0 : int(16 * scale));
    QRect rect(0, 0, this->width() - rectW, height);

    // draw text
    int offset = int(scale * 8);
    QRect textRect(offset, 0, this->width() - offset - offset, height);
    translateRectForLocation(textRect, this->tabLocation_,
                             this->selected_ ? -1 : -2);

    if (this->shouldDrawXButton())
    {
        textRect.setRight(textRect.right() - this->height() / 2);
    }

    int width = metrics.horizontalAdvance(this->getTitle());
    Qt::Alignment alignment = width > textRect.width()
                                  ? Qt::AlignLeft | Qt::AlignVCenter
                                  : Qt::AlignHCenter | Qt::AlignVCenter;

    QTextOption option(alignment);
    option.setWrapMode(QTextOption::NoWrap);
    painter.drawText(textRect, this->getTitle(), option);

    // draw close x
    if (this->shouldDrawXButton())
    {
        painter.setRenderHint(QPainter::Antialiasing, false);

        QRect xRect = this->getXRect();
        if (!xRect.isNull())
        {
            painter.setBrush(QColor("#fff"));

            if (this->mouseOverX_)
            {
                painter.fillRect(xRect, QColor(0, 0, 0, 64));

                if (this->mouseDownX_)
                {
                    painter.fillRect(xRect, QColor(0, 0, 0, 64));
                }
            }

            int a = static_cast<int>(scale * 4);

            painter.drawLine(xRect.topLeft() + QPoint(a, a),
                             xRect.bottomRight() + QPoint(-a, -a));
            painter.drawLine(xRect.topRight() + QPoint(-a, a),
                             xRect.bottomLeft() + QPoint(a, -a));
        }
    }

    // draw mouse over effect
    if (!this->selected_)
    {
        this->fancyPaint(painter);
    }

    // draw line at border
    if (!this->selected_ && this->isInLastRow_)
    {
        QRect borderRect;
        switch (this->tabLocation_)
        {
            case NotebookTabLocation::Top:
                borderRect = QRect(0, this->height() - 1, this->width(), 1);
                break;
            case NotebookTabLocation::Left:
                borderRect = QRect(this->width() - 1, 0, 1, this->height());
                break;
            case NotebookTabLocation::Right:
                borderRect = QRect(0, 0, 1, this->height());
                break;
            case NotebookTabLocation::Bottom:
                borderRect = QRect(0, 0, this->width(), 1);
                break;
        }
        painter.fillRect(borderRect, app->getThemes()->window.background);
    }
}

bool NotebookTab::hasXButton() const
{
    return getSettings()->showTabCloseButton &&
           this->notebook_->getAllowUserTabManagement();
}

bool NotebookTab::shouldDrawXButton() const
{
    return this->hasXButton() && (this->mouseOver_ || this->selected_);
}

void NotebookTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        this->mouseDown_ = true;
        this->mouseDownX_ = this->getXRect().contains(event->pos());

        this->notebook_->select(page);
    }

    this->update();

    if (this->notebook_->getAllowUserTabManagement())
    {
        switch (event->button())
        {
            case Qt::RightButton: {
                this->menu_.popup(event->globalPos() + QPoint(0, 8));
            }
            break;
            default:;
        }
    }
}

void NotebookTab::mouseReleaseEvent(QMouseEvent *event)
{
    this->mouseDown_ = false;

    auto removeThisPage = [this] {
        auto reply = QMessageBox::question(
            this, "Remove this tab",
            "Are you sure that you want to remove this tab?",
            QMessageBox::Yes | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
        {
            this->notebook_->removePage(this->page);
        }
    };

    if (event->button() == Qt::MiddleButton &&
        this->notebook_->getAllowUserTabManagement())
    {
        if (this->rect().contains(event->pos()))
        {
            removeThisPage();
        }
    }
    else
    {
        if (this->hasXButton() && this->mouseDownX_ &&
            this->getXRect().contains(event->pos()))
        {
            this->mouseDownX_ = false;

            removeThisPage();
        }
        else
        {
            this->update();
        }
    }
}

void NotebookTab::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton &&
        this->notebook_->getAllowUserTabManagement())
    {
        this->showRenameDialog();
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void NotebookTab::enterEvent(QEnterEvent *event)
#else
void NotebookTab::enterEvent(QEvent *event)
#endif
{
    this->mouseOver_ = true;

    this->update();

    Button::enterEvent(event);
}

void NotebookTab::leaveEvent(QEvent *event)
{
    this->mouseOverX_ = false;
    this->mouseOver_ = false;

    this->update();

    Button::leaveEvent(event);
}

void NotebookTab::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
    {
        return;
    }

    if (!isDraggingSplit())
    {
        // Ensure dragging a split from a different Chatterino instance doesn't switch tabs around
        return;
    }

    if (this->notebook_->getAllowUserTabManagement())
    {
        this->notebook_->select(this->page);
    }
}

void NotebookTab::mouseMoveEvent(QMouseEvent *event)
{
    if (getSettings()->showTabCloseButton &&
        this->notebook_->getAllowUserTabManagement())
    {
        bool overX = this->getXRect().contains(event->pos());

        if (overX != this->mouseOverX_)
        {
            // Over X state has been changed (we either left or entered it;
            this->mouseOverX_ = overX;

            this->update();
        }
    }

    QPoint relPoint = this->mapToParent(event->pos());

    if (this->mouseDown_ && !this->getDesiredRect().contains(relPoint) &&
        this->notebook_->getAllowUserTabManagement())
    {
        int index;
        QWidget *clickedPage =
            this->notebook_->tabAt(relPoint, index, this->width());

        if (clickedPage != nullptr && clickedPage != this->page)
        {
            this->notebook_->rearrangePage(this->page, index);
        }
    }

    Button::mouseMoveEvent(event);
}

void NotebookTab::wheelEvent(QWheelEvent *event)
{
    const auto defaultMouseDelta = 120;
    const auto verticalDelta = event->angleDelta().y();
    const auto selectTab = [this](int delta) {
        delta > 0 ? this->notebook_->selectPreviousTab()
                  : this->notebook_->selectNextTab();
    };
    // If it's true
    // Then the user uses the trackpad or perhaps the most accurate mouse
    // Which has small delta.
    if (std::abs(verticalDelta) < defaultMouseDelta)
    {
        this->mouseWheelDelta_ += verticalDelta;
        if (std::abs(this->mouseWheelDelta_) >= defaultMouseDelta)
        {
            selectTab(this->mouseWheelDelta_);
            this->mouseWheelDelta_ = 0;
        }
    }
    else
    {
        selectTab(verticalDelta);
    }
}

void NotebookTab::update()
{
    Button::update();
}

QRect NotebookTab::getXRect() const
{
    QRect rect = this->rect();
    float s = this->scale();
    int size = static_cast<int>(16 * s);

    int centerAdjustment = this->tabLocation_ == NotebookTabLocation::Top
                               ? (size / 3)   // slightly off true center
                               : (size / 2);  // true center

    QRect xRect(rect.right() - static_cast<int>(20 * s),
                rect.center().y() - centerAdjustment, size, size);

    if (this->selected_)
    {
        translateRectForLocation(xRect, this->tabLocation_, 1);
    }

    return xRect;
}

}  // namespace chatterino
