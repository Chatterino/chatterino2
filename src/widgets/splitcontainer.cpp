#include "widgets/splitcontainer.hpp"
#include "application.hpp"
#include "common.hpp"
#include "singletons/thememanager.hpp"
#include "util/helpers.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/helper/notebooktab.hpp"
#include "widgets/notebook.hpp"
#include "widgets/split.hpp"

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QMimeData>
#include <QObject>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/foreach.hpp>

#include <algorithm>

namespace chatterino {
namespace widgets {

bool SplitContainer::isDraggingSplit = false;
Split *SplitContainer::draggingSplit = nullptr;
// SplitContainer::Position SplitContainer::dragOriginalPosition;

SplitContainer::SplitContainer(Notebook *parent, NotebookTab *_tab)
    : BaseWidget(parent)
    , tab(_tab)
    , dropPreview(this)
    , mouseOverPoint(-10000, -10000)
    , overlay(this)
{
    this->tab->page = this;

    this->refreshTabTitle();

    this->managedConnect(Split::modifierStatusChanged, [this](auto) {
        // fourtf: maybe optimize
        this->layout();
    });

    this->setCursor(Qt::PointingHandCursor);
    this->setAcceptDrops(true);

    this->managedConnect(this->overlay.dragEnded, [this]() {
        this->isDragging = false;
        this->layout();
    });

    this->overlay.hide();

    this->setMouseTracking(true);
    this->setAcceptDrops(true);
}

NotebookTab *SplitContainer::getTab() const
{
    return this->tab;
}

void SplitContainer::appendNewSplit(bool openChannelNameDialog)
{
    Split *split = new Split(this);
    this->appendSplit(split);

    if (openChannelNameDialog) {
        split->showChangeChannelPopup("Open channel name", true, [=](bool ok) {
            if (!ok) {
                this->deleteSplit(split);
            }
        });
    }
}

void SplitContainer::appendSplit(Split *split)
{
    this->insertSplit(split, Direction::Right);
}

void SplitContainer::insertSplit(Split *split, const Position &position)
{
    this->insertSplit(split, position.direction, position.relativeNode);
}

void SplitContainer::insertSplit(Split *split, Direction direction, Split *relativeTo)
{
    Node *node = this->baseNode.findNodeContainingSplit(relativeTo);
    assert(node != nullptr);

    this->insertSplit(split, direction, node);
}

void SplitContainer::insertSplit(Split *split, Direction direction, Node *relativeTo)
{
    split->setContainer(this);

    if (relativeTo == nullptr) {
        if (this->baseNode.type == Node::EmptyRoot) {
            this->baseNode.setSplit(split);
        } else if (this->baseNode.type == Node::_Split) {
            this->baseNode.nestSplitIntoCollection(split, direction);
        } else {
            this->baseNode.insertSplitRelative(split, direction);
        }
    } else {
        assert(this->baseNode.isOrContainsNode(relativeTo));

        relativeTo->insertSplitRelative(split, direction);
    }

    split->setParent(this);
    split->show();
    split->giveFocus(Qt::MouseFocusReason);
    this->splits.push_back(split);

    //    this->setAcceptDrops(false);

    this->layout();
}

SplitContainer::Position SplitContainer::releaseSplit(Split *split)
{
    Node *node = this->baseNode.findNodeContainingSplit(split);
    assert(node != nullptr);

    this->splits.erase(std::find(this->splits.begin(), this->splits.end(), split));
    split->setParent(nullptr);
    Position position = node->releaseSplit();
    this->layout();
    if (splits.size() != 0) {
        this->splits.front()->giveFocus(Qt::MouseFocusReason);
    }

    //    if (this->splits.empty()) {
    //        this->setAcceptDrops(true);
    //    }

    return position;
}

SplitContainer::Position SplitContainer::deleteSplit(Split *split)
{
    assert(split != nullptr);

    split->deleteLater();
    return releaseSplit(split);
}

void SplitContainer::layout()
{
    this->baseNode.geometry = this->rect();

    std::vector<DropRect> _dropRects;
    this->baseNode.layout(
        Split::modifierStatus == (Qt::AltModifier | Qt::ControlModifier) || this->isDragging,
        this->getScale(), _dropRects);
    _dropRects.clear();
    this->baseNode.layout(
        Split::modifierStatus == (Qt::AltModifier | Qt::ControlModifier) || this->isDragging,
        this->getScale(), _dropRects);

    this->dropRects = _dropRects;

    for (Split *split : this->splits) {
        const QRect &g = split->geometry();

        Node *node = this->baseNode.findNodeContainingSplit(split);

        _dropRects.push_back(DropRect(QRect(g.left(), g.top(), g.width() / 4, g.height()),
                                      Position(node, Direction::Left)));
        _dropRects.push_back(
            DropRect(QRect(g.left() + g.width() / 4 * 3, g.top(), g.width() / 4, g.height()),
                     Position(node, Direction::Right)));

        _dropRects.push_back(DropRect(QRect(g.left(), g.top(), g.width(), g.height() / 2),
                                      Position(node, Direction::Above)));
        _dropRects.push_back(
            DropRect(QRect(g.left(), g.top() + g.height() / 2, g.width(), g.height() / 2),
                     Position(node, Direction::Below)));
    }

    if (this->splits.empty()) {
        QRect g = this->rect();
        _dropRects.push_back(DropRect(QRect(g.left(), g.top(), g.width(), g.height()),
                                      Position(nullptr, Direction::Below)));
    }

    this->overlay.setRects(std::move(_dropRects));
    this->update();
}

/// EVENTS
void SplitContainer::resizeEvent(QResizeEvent *event)
{
    BaseWidget::resizeEvent(event);

    this->layout();
}

void SplitContainer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (this->splits.size() == 0) {
            // "Add Chat" was clicked
            this->appendNewSplit(true);

            //            this->setCursor(QCursor(Qt::ArrowCursor));
        } else {
            auto it =
                std::find_if(this->dropRects.begin(), this->dropRects.end(),
                             [event](DropRect &rect) { return rect.rect.contains(event->pos()); });
            if (it != this->dropRects.end()) {
                this->insertSplit(new Split(this), it->position);
            }
        }
    }
}

void SplitContainer::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (this->splits.size() == 0) {
        painter.fillRect(rect(), this->themeManager->splits.background);

        painter.setPen(this->themeManager->splits.header.text);

        QString text = "Click to add a <split>";

        Notebook *notebook = dynamic_cast<Notebook *>(this->parentWidget());

        if (notebook != nullptr) {
            if (notebook->tabCount() > 1) {
                text += "\n\ntip: you can drag a <split> while holding <Alt>";
                text += "\nor add another one by pressing <Ctrl+T>";
            }
        }

        painter.drawText(rect(), text, QTextOption(Qt::AlignCenter));
    } else {
        painter.fillRect(rect(), this->themeManager->splits.messageSeperator);
    }

    for (DropRect &dropRect : this->dropRects) {
        QColor border = getApp()->themes->splits.dropPreviewBorder;
        QColor background = getApp()->themes->splits.dropPreview;

        if (!dropRect.rect.contains(this->mouseOverPoint)) {
            //            border.setAlphaF(0.1);
            background.setAlphaF(0.1);
        }

        painter.setPen(border);
        painter.setBrush(background);

        painter.drawRect(dropRect.rect.marginsRemoved(QMargins(2, 2, 2, 2)));
    }

    QBrush accentColor = (QApplication::activeWindow() == this->window()
                              ? this->themeManager->tabs.selected.backgrounds.regular
                              : this->themeManager->tabs.selected.backgrounds.unfocused);

    painter.fillRect(0, 0, width(), 1, accentColor);
}

void SplitContainer::dragEnterEvent(QDragEnterEvent *event)
{
    //    if (!event->mimeData()->hasFormat("chatterino/split"))
    //        return;

    //    if (!SplitContainer::isDraggingSplit) {
    //        return;
    //    }

    this->isDragging = true;
    this->layout();

    //    if (this->splits.empty()) {
    //        event->acceptProposedAction();
    //    }

    this->overlay.setGeometry(this->rect());
    this->overlay.show();
    this->overlay.raise();
}

void SplitContainer::dragMoveEvent(QDragMoveEvent *event)
{
    //    if (this->splits.empty()) {
    //        event->acceptProposedAction();
    //    }

    //    for (auto &dropRect : this->dropRects) {
    //        if (dropRect.rect.contains(event->pos())) {
    //            event->acceptProposedAction();
    //            return;
    //        }
    //    }
    //    event->setAccepted(false);
}

void SplitContainer::dragLeaveEvent(QDragLeaveEvent *event)
{
    //    this->isDragging = false;
    //    this->layout();
}

void SplitContainer::dropEvent(QDropEvent *event)
{
    //    if (this->splits.empty()) {
    //        this->insertSplit(SplitContainer::draggingSplit, Direction::Above);
    //        event->acceptProposedAction();
    //    }

    //    this->isDragging = false;
    //    this->layout();
}

// void SplitContainer::requestFocus(int requestedX, int requestedY)
//{
//    // XXX: Perhaps if we request an Y coordinate out of bounds, we shuold set all previously set
//    // requestedYs to 0 (if -1 is requested) or that x-coordinates vbox count (if requestedY >=
//    // currentvbox.count() is requested)
//    if (requestedX < 0 || requestedX >= this->ui.hbox.count()) {
//        return;
//    }

//    QLayoutItem *item = this->ui.hbox.itemAt(requestedX);
//    QWidget *xW = item->widget();
//    if (item->isEmpty()) {
//        qDebug() << "Requested hbox item " << requestedX << "is empty";
//        if (xW) {
//            qDebug() << "but xW is not null";
//            // TODO: figure out what to do here
//        }
//        return;
//    }

//    QVBoxLayout *vbox = static_cast<QVBoxLayout *>(item->layout());

//    if (requestedY < 0) {
//        requestedY = 0;
//    } else if (requestedY >= vbox->count()) {
//        requestedY = vbox->count() - 1;
//    }

//    this->lastRequestedY[requestedX] = requestedY;

//    QLayoutItem *innerItem = vbox->itemAt(requestedY);

//    if (innerItem->isEmpty()) {
//        qDebug() << "Requested vbox item " << requestedY << "is empty";
//        return;
//    }

//    QWidget *w = innerItem->widget();
//    if (w) {
//        Split *chatWidget = static_cast<Split *>(w);
//        chatWidget->giveFocus(Qt::OtherFocusReason);
//    }
//}

// void SplitContainer::enterEvent(QEvent *event)
//{
//    if (this->ui.hbox.count() == 0) {
//        this->setCursor(QCursor(Qt::PointingHandCursor));
//    } else {
//        this->setCursor(QCursor(Qt::ArrowCursor));
//    }
//}

void SplitContainer::mouseMoveEvent(QMouseEvent *event)
{
    this->mouseOverPoint = event->pos();
    this->update();
}

void SplitContainer::leaveEvent(QEvent *event)
{
    this->mouseOverPoint = QPoint(-1000, -10000);
    this->update();
}

// void SplitContainer::setPreviewRect(QPoint mousePos)
//{
//    for (DropRegion region : this->dropRegions) {
//        if (region.rect.contains(mousePos)) {
//            this->dropPreview.setBounds(region.rect);

//            if (!this->dropPreview.isVisible()) {
//                this->dropPreview.setGeometry(this->rect());
//                this->dropPreview.show();
//                this->dropPreview.raise();
//            }

//            dropPosition = region.position;

//            return;
//        }
//    }

//    this->dropPreview.hide();
//}

// bool SplitContainer::eventFilter(QObject *object, QEvent *event)
//{
//    if (event->type() == QEvent::FocusIn) {
//        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);

//        this->refreshCurrentFocusCoordinates((focusEvent->reason() == Qt::MouseFocusReason));
//    }

//    return false;
//}

// void SplitContainer::showEvent(QShowEvent *event)
//{
//    // Whenever this notebook page is shown, give focus to the last focused chat widget
//    // If this is the first time this notebook page is shown, it will give focus to the
//    top-left
//    // chat widget
//    this->requestFocus(this->currentX, this->currentY);
//}

// static std::pair<int, int> getWidgetPositionInLayout(QLayout *layout, const Split
// *chatWidget)
//{
//    for (int i = 0; i < layout->count(); ++i) {
//        printf("xD\n");
//    }

//    return std::make_pair(-1, -1);
//}

// std::pair<int, int> SplitContainer::getChatPosition(const Split *chatWidget)
//{
//    auto layout = this->ui.hbox.layout();

//    if (layout == nullptr) {
//        return std::make_pair(-1, -1);
//    }

//    return getWidgetPositionInLayout(layout, chatWidget);
//}

// Split *SplitContainer::createChatWidget()
//{
//    auto split = new Split(this);

//    return split;
//}

void SplitContainer::refreshTabTitle()
{
    assert(this->tab != nullptr);

    if (!this->tab->useDefaultTitle) {
        return;
    }

    this->tab->setTitle("default title");

    //    QString newTitle = "";
    //    bool first = true;

    //    for (const auto &chatWidget : this->splits) {
    //        auto channelName = chatWidget->getChannel()->name;
    //        if (channelName.isEmpty()) {
    //            continue;
    //        }

    //        if (!first) {
    //            newTitle += ", ";
    //        }
    //        newTitle += channelName;

    //        first = false;
    //    }

    //    if (newTitle.isEmpty()) {
    //        newTitle = "empty";
    //    }

    //    this->tab->setTitle(newTitle);
}

}  // namespace widgets
}  // namespace chatterino
