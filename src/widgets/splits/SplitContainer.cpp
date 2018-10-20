#include "widgets/splits/SplitContainer.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QMimeData>
#include <QObject>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <boost/foreach.hpp>

namespace chatterino {

bool SplitContainer::isDraggingSplit = false;
Split *SplitContainer::draggingSplit = nullptr;

SplitContainer::SplitContainer(Notebook *parent)
    : BaseWidget(parent)
    , overlay_(this)
    , mouseOverPoint_(-10000, -10000)
    , tab_(nullptr)
{
    this->refreshTabTitle();

    this->managedConnect(Split::modifierStatusChanged, [this](auto modifiers) {
        this->layout();

        if (modifiers == showResizeHandlesModifiers) {
            for (auto &handle : this->resizeHandles_) {
                handle->show();
                handle->raise();
            }
        } else {
            for (auto &handle : this->resizeHandles_) {
                handle->hide();
            }
        }

        if (modifiers == showSplitOverlayModifiers) {
            this->setCursor(Qt::PointingHandCursor);
        } else {
            this->unsetCursor();
        }
    });

    this->setCursor(Qt::PointingHandCursor);
    this->setAcceptDrops(true);

    this->managedConnect(this->overlay_.dragEnded, [this]() {
        this->isDragging_ = false;
        this->layout();
    });

    this->overlay_.hide();

    this->setMouseTracking(true);
    this->setAcceptDrops(true);
}

NotebookTab *SplitContainer::getTab() const
{
    return this->tab_;
}

void SplitContainer::setTab(NotebookTab *_tab)
{
    this->tab_ = _tab;

    this->tab_->page = this;

    this->refreshTab();
}

void SplitContainer::hideResizeHandles()
{
    this->overlay_.hide();

    for (auto &handle : this->resizeHandles_) {
        handle->hide();
    }
}

void SplitContainer::resetMouseStatus()
{
    this->mouseOverPoint_ = QPoint(-10000, -10000);
    this->update();
}

Split *SplitContainer::appendNewSplit(bool openChannelNameDialog)
{
    assertInGuiThread();

    Split *split = new Split(this);
    this->appendSplit(split);

    if (openChannelNameDialog) {
        split->showChangeChannelPopup("Open channel name", true, [=](bool ok) {
            if (!ok) {
                this->deleteSplit(split);
            }
        });
    }

    return split;
}

void SplitContainer::appendSplit(Split *split)
{
    this->insertSplit(split, Direction::Right);
}

void SplitContainer::insertSplit(Split *split, const Position &position)
{
    this->insertSplit(split, position.direction_,
                      reinterpret_cast<Node *>(position.relativeNode_));
}

void SplitContainer::insertSplit(Split *split, Direction direction,
                                 Split *relativeTo)
{
    Node *node = this->baseNode_.findNodeContainingSplit(relativeTo);
    assert(node != nullptr);

    this->insertSplit(split, direction, node);
}

void SplitContainer::insertSplit(Split *split, Direction direction,
                                 Node *relativeTo)
{
    // Queue up save because: Split added
    getApp()->windows->queueSave();

    assertInGuiThread();

    split->setContainer(this);

    if (relativeTo == nullptr) {
        if (this->baseNode_.type_ == Node::EmptyRoot) {
            this->baseNode_.setSplit(split);
        } else if (this->baseNode_.type_ == Node::_Split) {
            this->baseNode_.nestSplitIntoCollection(split, direction);
        } else {
            this->baseNode_.insertSplitRelative(split, direction);
        }
    } else {
        assert(this->baseNode_.isOrContainsNode(relativeTo));

        relativeTo->insertSplitRelative(split, direction);
    }

    this->addSplit(split);
}

void SplitContainer::addSplit(Split *split)
{
    assertInGuiThread();

    split->setParent(this);
    split->show();
    split->giveFocus(Qt::MouseFocusReason);
    this->unsetCursor();
    this->splits_.push_back(split);

    this->refreshTab();

    split->getChannelView().tabHighlightRequested.connect(
        [this](HighlightState state) {
            if (this->tab_ != nullptr) {
                this->tab_->setHighlightState(state);
            }
        });

    split->getChannelView().liveStatusChanged.connect([this]() {
        this->refreshTabLiveStatus();  //
    });

    split->focused.connect([this, split] { this->setSelected(split); });

    this->layout();
}

void SplitContainer::setSelected(Split *split)
{
    this->selected_ = split;

    if (Node *node = this->baseNode_.findNodeContainingSplit(split)) {
        this->setPreferedTargetRecursive(node);
    }
}

void SplitContainer::setPreferedTargetRecursive(Node *node)
{
    if (node->parent_ != nullptr) {
        node->parent_->preferedFocusTarget_ = node;

        this->setPreferedTargetRecursive(node->parent_);
    }
}

SplitContainer::Position SplitContainer::releaseSplit(Split *split)
{
    assertInGuiThread();

    Node *node = this->baseNode_.findNodeContainingSplit(split);
    assert(node != nullptr);

    this->splits_.erase(
        std::find(this->splits_.begin(), this->splits_.end(), split));
    split->setParent(nullptr);
    Position position = node->releaseSplit();
    this->layout();
    if (splits_.size() == 0) {
        this->setSelected(nullptr);
        this->setCursor(Qt::PointingHandCursor);
    } else {
        this->splits_.front()->giveFocus(Qt::MouseFocusReason);
    }

    this->refreshTab();

    // fourtf: really bad
    split->getChannelView().tabHighlightRequested.disconnectAll();

    split->getChannelView().tabHighlightRequested.disconnectAll();

    return position;
}

SplitContainer::Position SplitContainer::deleteSplit(Split *split)
{
    // Queue up save because: Split removed
    getApp()->windows->queueSave();

    assertInGuiThread();
    assert(split != nullptr);

    split->deleteLater();
    return releaseSplit(split);
}

void SplitContainer::selectNextSplit(Direction direction)
{
    assertInGuiThread();

    if (Node *node = this->baseNode_.findNodeContainingSplit(this->selected_)) {
        this->selectSplitRecursive(node, direction);
    }
}

void SplitContainer::selectSplitRecursive(Node *node, Direction direction)
{
    if (node->parent_ != nullptr) {
        if (node->parent_->type_ == Node::toContainerType(direction)) {
            auto &siblings = node->parent_->children_;

            auto it = std::find_if(
                siblings.begin(), siblings.end(),
                [node](const auto &other) { return other.get() == node; });
            assert(it != siblings.end());

            if (direction == Direction::Left || direction == Direction::Above) {
                if (it == siblings.begin()) {
                    this->selectSplitRecursive(node->parent_, direction);
                } else {
                    this->focusSplitRecursive(
                        siblings[it - siblings.begin() - 1].get(), direction);
                }
            } else {
                if (it->get() == siblings.back().get()) {
                    this->selectSplitRecursive(node->parent_, direction);
                } else {
                    this->focusSplitRecursive(
                        siblings[it - siblings.begin() + 1].get(), direction);
                }
            }
        } else {
            this->selectSplitRecursive(node->parent_, direction);
        }
    }
}

void SplitContainer::focusSplitRecursive(Node *node, Direction direction)
{
    switch (node->type_) {
        case Node::_Split: {
            node->split_->giveFocus(Qt::OtherFocusReason);
        } break;

        case Node::HorizontalContainer:
        case Node::VerticalContainer: {
            auto &children = node->children_;

            auto it = std::find_if(
                children.begin(), children.end(), [node](const auto &other) {
                    return node->preferedFocusTarget_ == other.get();
                });

            if (it != children.end()) {
                this->focusSplitRecursive(it->get(), direction);
            } else {
                this->focusSplitRecursive(node->children_.front().get(),
                                          direction);
            }
        } break;

        default:;
    }
}

Split *SplitContainer::getTopRightSplit(Node &node)
{
    switch (node.getType()) {
        case Node::_Split:
            return node.getSplit();
        case Node::VerticalContainer:
            if (!node.getChildren().empty())
                return getTopRightSplit(*node.getChildren().front());
            break;
        case Node::HorizontalContainer:
            if (!node.getChildren().empty())
                return getTopRightSplit(*node.getChildren().back());
            break;
        default:;
    }
    return nullptr;
}

void SplitContainer::layout()
{
    // update top right split
    auto topRight = this->getTopRightSplit(this->baseNode_);
    if (this->topRight_) this->topRight_->setIsTopRightSplit(false);
    this->topRight_ = topRight;
    if (topRight) this->topRight_->setIsTopRightSplit(true);

    // layout
    this->baseNode_.geometry_ = this->rect().adjusted(-1, -1, 0, 0);

    std::vector<DropRect> _dropRects;
    std::vector<ResizeRect> _resizeRects;
    this->baseNode_.layout(
        Split::modifierStatus == showAddSplitRegions || this->isDragging_,
        this->getScale(), _dropRects, _resizeRects);

    this->dropRects_ = _dropRects;

    for (Split *split : this->splits_) {
        const QRect &g = split->geometry();

        Node *node = this->baseNode_.findNodeContainingSplit(split);

        // left
        _dropRects.push_back(
            DropRect(QRect(g.left(), g.top(), g.width() / 3, g.height()),
                     Position(node, Direction::Left)));
        // right
        _dropRects.push_back(DropRect(QRect(g.right() - g.width() / 3, g.top(),
                                            g.width() / 3, g.height()),
                                      Position(node, Direction::Right)));

        // top
        _dropRects.push_back(
            DropRect(QRect(g.left(), g.top(), g.width(), g.height() / 2),
                     Position(node, Direction::Above)));
        // bottom
        _dropRects.push_back(
            DropRect(QRect(g.left(), g.bottom() - g.height() / 2, g.width(),
                           g.height() / 2),
                     Position(node, Direction::Below)));
    }

    if (this->splits_.empty()) {
        QRect g = this->rect();
        _dropRects.push_back(
            DropRect(QRect(g.left(), g.top(), g.width() - 1, g.height() - 1),
                     Position(nullptr, Direction::Below)));
    }

    this->overlay_.setRects(std::move(_dropRects));

    // handle resizeHandles
    if (this->resizeHandles_.size() < _resizeRects.size()) {
        while (this->resizeHandles_.size() < _resizeRects.size()) {
            this->resizeHandles_.push_back(
                std::make_unique<ResizeHandle>(this));
        }
    } else if (this->resizeHandles_.size() > _resizeRects.size()) {
        this->resizeHandles_.resize(_resizeRects.size());
    }

    {
        size_t i = 0;
        for (ResizeRect &resizeRect : _resizeRects) {
            ResizeHandle *handle = this->resizeHandles_[i].get();
            handle->setGeometry(resizeRect.rect);
            handle->setVertical(resizeRect.vertical);
            handle->node = resizeRect.node;

            if (Split::modifierStatus == showResizeHandlesModifiers) {
                handle->show();
                handle->raise();
            }

            i++;
        }
    }

    // redraw
    this->update();
}

void SplitContainer::resizeEvent(QResizeEvent *event)
{
    BaseWidget::resizeEvent(event);

    this->layout();
}

void SplitContainer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (this->splits_.size() == 0) {
            // "Add Chat" was clicked
            this->appendNewSplit(true);
            this->mouseOverPoint_ = QPoint(-10000, -10000);

            //            this->setCursor(QCursor(Qt::ArrowCursor));
        } else {
            auto it =
                std::find_if(this->dropRects_.begin(), this->dropRects_.end(),
                             [event](DropRect &rect) {
                                 return rect.rect.contains(event->pos());
                             });
            if (it != this->dropRects_.end()) {
                this->insertSplit(new Split(this), it->position);
            }
        }
    }
}

void SplitContainer::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (this->splits_.size() == 0) {
        painter.fillRect(rect(), this->theme->splits.background);

        painter.setPen(this->theme->splits.header.text);

        QString text = "Click to add a split";

        Notebook *notebook = dynamic_cast<Notebook *>(this->parentWidget());

        if (notebook != nullptr) {
            if (notebook->getPageCount() > 1) {
                text += "\n\nAfter adding hold <Ctrl+Alt> to move or split it.";
            }
        }

        painter.drawText(rect(), text, QTextOption(Qt::AlignCenter));
    } else {
        if (getApp()->themes->isLightTheme()) {
            painter.fillRect(rect(), QColor("#999"));
        } else {
            painter.fillRect(rect(), QColor("#555"));
        }
    }

    for (DropRect &dropRect : this->dropRects_) {
        QColor border = getApp()->themes->splits.dropTargetRectBorder;
        QColor background = getApp()->themes->splits.dropTargetRect;

        if (!dropRect.rect.contains(this->mouseOverPoint_)) {
            //            border.setAlphaF(0.1);
            //            background.setAlphaF(0.1);
        } else {
            //            background.setAlphaF(0.1);
            border.setAlpha(255);
        }

        painter.setPen(border);
        painter.setBrush(background);

        auto rect = dropRect.rect.marginsRemoved(QMargins(2, 2, 2, 2));

        painter.drawRect(rect);

        int s =
            std::min<int>(dropRect.rect.width(), dropRect.rect.height()) - 12;

        if (this->theme->isLightTheme()) {
            painter.setPen(QColor(0, 0, 0));
        } else {
            painter.setPen(QColor(255, 255, 255));
        }
        painter.drawLine(rect.left() + rect.width() / 2 - (s / 2),
                         rect.top() + rect.height() / 2,
                         rect.left() + rect.width() / 2 + (s / 2),
                         rect.top() + rect.height() / 2);
        painter.drawLine(rect.left() + rect.width() / 2,
                         rect.top() + rect.height() / 2 - (s / 2),
                         rect.left() + rect.width() / 2,
                         rect.top() + rect.height() / 2 + (s / 2));
    }

    QBrush accentColor =
        (QApplication::activeWindow() == this->window()
             ? this->theme->tabs.selected.backgrounds.regular
             : this->theme->tabs.selected.backgrounds.unfocused);

    painter.fillRect(0, 0, width(), 1, accentColor);
}

void SplitContainer::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split")) return;

    if (!SplitContainer::isDraggingSplit) return;

    this->isDragging_ = true;
    this->layout();

    this->overlay_.setGeometry(this->rect());
    this->overlay_.show();
    this->overlay_.raise();
}

void SplitContainer::mouseMoveEvent(QMouseEvent *event)
{
    if (Split::modifierStatus == showSplitOverlayModifiers) {
        this->setCursor(Qt::PointingHandCursor);
    }

    this->mouseOverPoint_ = event->pos();
    this->update();
}

void SplitContainer::leaveEvent(QEvent *)
{
    this->mouseOverPoint_ = QPoint(-10000, -10000);
    this->update();
}

void SplitContainer::focusInEvent(QFocusEvent *)
{
    if (this->baseNode_.findNodeContainingSplit(this->selected_) != nullptr) {
        this->selected_->setFocus();
        return;
    }

    if (this->splits_.size() != 0) {
        this->splits_.front()->setFocus();
    }
}

void SplitContainer::refreshTab()
{
    this->refreshTabTitle();
    this->refreshTabLiveStatus();
}

int SplitContainer::getSplitCount()
{
    return 0;
}

const std::vector<Split *> SplitContainer::getSplits() const
{
    return this->splits_;
}

SplitContainer::Node *SplitContainer::getBaseNode()
{
    return &this->baseNode_;
}

void SplitContainer::decodeFromJson(QJsonObject &obj)
{
    assert(this->baseNode_.type_ == Node::EmptyRoot);

    this->decodeNodeRecusively(obj, &this->baseNode_);
}

void SplitContainer::decodeNodeRecusively(QJsonObject &obj, Node *node)
{
    QString type = obj.value("type").toString();

    if (type == "split") {
        auto *split = new Split(this);
        split->setChannel(
            WindowManager::decodeChannel(obj.value("data").toObject()));

        this->appendSplit(split);
    } else if (type == "horizontal" || type == "vertical") {
        bool vertical = type == "vertical";

        Direction direction = vertical ? Direction::Below : Direction::Right;

        node->type_ =
            vertical ? Node::VerticalContainer : Node::HorizontalContainer;

        for (QJsonValue _val : obj.value("items").toArray()) {
            auto _obj = _val.toObject();

            auto _type = _obj.value("type");
            if (_type == "split") {
                auto *split = new Split(this);
                split->setChannel(WindowManager::decodeChannel(
                    _obj.value("data").toObject()));

                Node *_node = new Node();
                _node->parent_ = node;
                _node->split_ = split;
                _node->type_ = Node::_Split;

                _node->flexH_ = _obj.value("flexh").toDouble(1.0);
                _node->flexV_ = _obj.value("flexv").toDouble(1.0);
                node->children_.emplace_back(_node);

                this->addSplit(split);
            } else {
                Node *_node = new Node();
                _node->parent_ = node;
                node->children_.emplace_back(_node);
                this->decodeNodeRecusively(_obj, _node);
            }
        }

        for (int i = 0; i < 2; i++) {
            if (node->getChildren().size() < 2) {
                auto *split = new Split(this);
                split->setChannel(
                    WindowManager::decodeChannel(obj.value("data").toObject()));

                this->insertSplit(split, direction, node);
            }
        }
    }
}

void SplitContainer::refreshTabTitle()
{
    if (this->tab_ == nullptr) {
        return;
    }

    QString newTitle = "";
    bool first = true;

    for (const auto &chatWidget : this->splits_) {
        auto channelName = chatWidget->getChannel()->getName();
        if (channelName.isEmpty()) {
            continue;
        }

        if (!first) {
            newTitle += ", ";
        }
        newTitle += channelName;

        first = false;
    }

    if (newTitle.isEmpty()) {
        newTitle = "empty";
    }

    this->tab_->setDefaultTitle(newTitle);
}

void SplitContainer::refreshTabLiveStatus()
{
    if (this->tab_ == nullptr) {
        return;
    }

    bool liveStatus = false;
    for (const auto &s : this->splits_) {
        auto c = s->getChannel();
        if (c->isLive()) {
            liveStatus = true;
            break;
        }
    }

    this->tab_->setLive(liveStatus);
}

//
// Node
//

SplitContainer::Node::Type SplitContainer::Node::getType()
{
    return this->type_;
}
Split *SplitContainer::Node::getSplit()
{
    return this->split_;
}

SplitContainer::Node *SplitContainer::Node::getParent()
{
    return this->parent_;
}

qreal SplitContainer::Node::getHorizontalFlex()
{
    return this->flexH_;
}

qreal SplitContainer::Node::getVerticalFlex()
{
    return this->flexV_;
}

const std::vector<std::unique_ptr<SplitContainer::Node>>
    &SplitContainer::Node::getChildren()
{
    return this->children_;
}

SplitContainer::Node::Node()
    : type_(SplitContainer::Node::Type::EmptyRoot)
    , split_(nullptr)
    , parent_(nullptr)
{
}

SplitContainer::Node::Node(Split *_split, Node *_parent)
    : type_(Type::_Split)
    , split_(_split)
    , parent_(_parent)
{
}

bool SplitContainer::Node::isOrContainsNode(SplitContainer::Node *_node)
{
    if (this == _node) {
        return true;
    }

    return std::any_of(this->children_.begin(), this->children_.end(),
                       [_node](std::unique_ptr<Node> &n) {
                           return n->isOrContainsNode(_node);
                       });
}

SplitContainer::Node *SplitContainer::Node::findNodeContainingSplit(
    Split *_split)
{
    if (this->type_ == Type::_Split && this->split_ == _split) {
        return this;
    }

    for (std::unique_ptr<Node> &node : this->children_) {
        Node *a = node->findNodeContainingSplit(_split);

        if (a != nullptr) {
            return a;
        }
    }
    return nullptr;
}

void SplitContainer::Node::insertSplitRelative(Split *_split,
                                               Direction _direction)
{
    if (this->parent_ == nullptr) {
        switch (this->type_) {
            case Node::EmptyRoot: {
                this->setSplit(_split);
            } break;
            case Node::_Split: {
                this->nestSplitIntoCollection(_split, _direction);
            } break;
            case Node::HorizontalContainer: {
                this->nestSplitIntoCollection(_split, _direction);
            } break;
            case Node::VerticalContainer: {
                this->nestSplitIntoCollection(_split, _direction);
            } break;
        }
        return;
    }

    // parent != nullptr
    if (parent_->type_ == toContainerType(_direction)) {
        // hell yeah we'll just insert it next to outselves
        this->insertNextToThis(_split, _direction);
    } else {
        this->nestSplitIntoCollection(_split, _direction);
    }
}

void SplitContainer::Node::nestSplitIntoCollection(Split *_split,
                                                   Direction _direction)
{
    if (toContainerType(_direction) == this->type_) {
        this->children_.emplace_back(new Node(_split, this));
    } else {
        // we'll need to nest outselves
        // move all our data into a new node
        Node *clone = new Node();
        clone->type_ = this->type_;
        clone->children_ = std::move(this->children_);
        for (std::unique_ptr<Node> &node : clone->children_) {
            node->parent_ = clone;
        }
        clone->split_ = this->split_;
        clone->parent_ = this;

        // add the node to our children and change our type
        this->children_.push_back(std::unique_ptr<Node>(clone));
        this->type_ = toContainerType(_direction);
        this->split_ = nullptr;

        clone->insertNextToThis(_split, _direction);
    }
}

void SplitContainer::Node::insertNextToThis(Split *_split, Direction _direction)
{
    auto &siblings = this->parent_->children_;

    qreal width = this->parent_->geometry_.width() / siblings.size();
    qreal height = this->parent_->geometry_.height() / siblings.size();

    if (siblings.size() == 1) {
        this->geometry_ = QRect(0, 0, int(width), int(height));
    }

    auto it = std::find_if(siblings.begin(), siblings.end(),
                           [this](auto &node) { return this == node.get(); });

    assert(it != siblings.end());
    if (_direction == Direction::Right || _direction == Direction::Below) {
        it++;
    }

    Node *node = new Node(_split, this->parent_);
    node->geometry_ = QRectF(0, 0, width, height);
    siblings.insert(it, std::unique_ptr<Node>(node));
}

void SplitContainer::Node::setSplit(Split *_split)
{
    assert(this->split_ == nullptr);
    assert(this->children_.size() == 0);

    this->split_ = _split;
    this->type_ = Type::_Split;
}

SplitContainer::Position SplitContainer::Node::releaseSplit()
{
    assert(this->type_ == Type::_Split);

    if (parent_ == nullptr) {
        this->type_ = Type::EmptyRoot;
        this->split_ = nullptr;

        Position pos;
        pos.relativeNode_ = nullptr;
        pos.direction_ = Direction::Right;
        return pos;
    } else {
        auto &siblings = this->parent_->children_;

        auto it =
            std::find_if(begin(siblings), end(siblings),
                         [this](auto &node) { return this == node.get(); });
        assert(it != siblings.end());

        Position position;
        if (siblings.size() == 2) {
            // delete this and move split to parent
            position.relativeNode_ = this->parent_;
            if (this->parent_->type_ == Type::VerticalContainer) {
                position.direction_ = siblings.begin() == it ? Direction::Above
                                                             : Direction::Below;
            } else {
                position.direction_ =
                    siblings.begin() == it ? Direction::Left : Direction::Right;
            }

            Node *_parent = this->parent_;
            siblings.erase(it);
            std::unique_ptr<Node> &sibling = siblings.front();
            _parent->type_ = sibling->type_;
            _parent->split_ = sibling->split_;
            std::vector<std::unique_ptr<Node>> nodes =
                std::move(sibling->children_);
            for (auto &node : nodes) {
                node->parent_ = _parent;
            }
            _parent->children_ = std::move(nodes);
        } else {
            if (this == siblings.back().get()) {
                position.direction_ =
                    this->parent_->type_ == Type::VerticalContainer
                        ? Direction::Below
                        : Direction::Right;
                siblings.erase(it);
                position.relativeNode_ = siblings.back().get();
            } else {
                position.relativeNode_ = (it + 1)->get();
                position.direction_ =
                    this->parent_->type_ == Type::VerticalContainer
                        ? Direction::Above
                        : Direction::Left;
                siblings.erase(it);
            }
        }

        return position;
    }
}

qreal SplitContainer::Node::getFlex(bool isVertical)
{
    return isVertical ? this->flexV_ : this->flexH_;
}

qreal SplitContainer::Node::getSize(bool isVertical)
{
    return isVertical ? this->geometry_.height() : this->geometry_.width();
}

qreal SplitContainer::Node::getChildrensTotalFlex(bool isVertical)
{
    return std::accumulate(this->children_.begin(), this->children_.end(),
                           qreal(0),
                           [=](qreal val, std::unique_ptr<Node> &node) {
                               return val + node->getFlex(isVertical);
                           });
}

void SplitContainer::Node::layout(bool addSpacing, float _scale,
                                  std::vector<DropRect> &dropRects,
                                  std::vector<ResizeRect> &resizeRects)
{
    for (std::unique_ptr<Node> &node : this->children_) {
        if (node->flexH_ <= 0) node->flexH_ = 0;
        if (node->flexV_ <= 0) node->flexV_ = 0;
    }

    switch (this->type_) {
        case Node::_Split: {
            QRect rect = this->geometry_.toRect();
            this->split_->setGeometry(
                rect.marginsRemoved(QMargins(1, 1, 0, 0)));
        } break;
        case Node::VerticalContainer:
        case Node::HorizontalContainer: {
            bool isVertical = this->type_ == Node::VerticalContainer;

            // vars
            qreal minSize = qreal(48 * _scale);

            qreal totalFlex = this->getChildrensTotalFlex(isVertical);
            qreal totalSize = std::accumulate(
                this->children_.begin(), this->children_.end(), qreal(0),
                [=](int val, std::unique_ptr<Node> &node) {
                    return val + std::max<qreal>(this->getSize(isVertical) /
                                                     totalFlex *
                                                     node->getFlex(isVertical),
                                                 minSize);
                });

            qreal sizeMultiplier = this->getSize(isVertical) / totalSize;
            QRectF childRect = this->geometry_;

            // add spacing if reqested
            if (addSpacing) {
                qreal offset = std::min<qreal>(this->getSize(!isVertical) * 0.1,
                                               qreal(_scale * 24));

                // droprect left / above
                dropRects.emplace_back(
                    QRectF(this->geometry_.left(), this->geometry_.top(),
                           isVertical ? offset : this->geometry_.width(),
                           isVertical ? this->geometry_.height() : offset)
                        .toRect(),
                    Position(this,
                             isVertical ? Direction::Left : Direction::Above));

                // droprect right / below
                if (isVertical) {
                    dropRects.emplace_back(
                        QRectF(this->geometry_.right() - offset,
                               this->geometry_.top(), offset,
                               this->geometry_.height())
                            .toRect(),
                        Position(this, Direction::Right));
                } else {
                    dropRects.emplace_back(
                        QRectF(this->geometry_.left(),
                               this->geometry_.bottom() - offset,
                               this->geometry_.width(), offset)
                            .toRect(),
                        Position(this, Direction::Below));
                }

                // shrink childRect
                if (isVertical) {
                    childRect.setLeft(childRect.left() + offset);
                    childRect.setRight(childRect.right() - offset);
                } else {
                    childRect.setTop(childRect.top() + offset);
                    childRect.setBottom(childRect.bottom() - offset);
                }
            }

            // iterate children
            auto pos = int(isVertical ? childRect.top() : childRect.left());
            for (std::unique_ptr<Node> &child : this->children_) {
                // set rect
                QRect rect = childRect.toRect();
                if (isVertical) {
                    rect.setTop(pos);
                    rect.setHeight(
                        std::max<qreal>(this->geometry_.height() / totalFlex *
                                            child->flexV_,
                                        minSize) *
                        sizeMultiplier);
                } else {
                    rect.setLeft(pos);
                    rect.setWidth(std::max<qreal>(this->geometry_.width() /
                                                      totalFlex * child->flexH_,
                                                  minSize) *
                                  sizeMultiplier);
                }

                if (child == this->children_.back()) {
                    rect.setRight(childRect.right() - 1);
                    rect.setBottom(childRect.bottom() - 1);
                }

                child->geometry_ = rect;
                child->layout(addSpacing, _scale, dropRects, resizeRects);

                pos += child->getSize(isVertical);

                // add resize rect
                if (child != this->children_.front()) {
                    QRectF r = isVertical ? QRectF(this->geometry_.left(),
                                                   child->geometry_.top() - 4,
                                                   this->geometry_.width(), 8)
                                          : QRectF(child->geometry_.left() - 4,
                                                   this->geometry_.top(), 8,
                                                   this->geometry_.height());
                    resizeRects.push_back(
                        ResizeRect(r.toRect(), child.get(), isVertical));
                }

                // normalize flex
                if (isVertical) {
                    child->flexV_ =
                        child->flexV_ / totalFlex * this->children_.size();
                    child->flexH_ = 1;
                } else {
                    child->flexH_ =
                        child->flexH_ / totalFlex * this->children_.size();
                    child->flexV_ = 1;
                }
            }
        } break;
    };
}

SplitContainer::Node::Type SplitContainer::Node::toContainerType(Direction _dir)
{
    return _dir == Direction::Left || _dir == Direction::Right
               ? Type::HorizontalContainer
               : Type::VerticalContainer;
}

//
// DropOverlay
//

SplitContainer::DropOverlay::DropOverlay(SplitContainer *_parent)
    : QWidget(_parent)
    , mouseOverPoint_(-10000, -10000)
    , parent_(_parent)
{
    this->setMouseTracking(true);
    this->setAcceptDrops(true);
}

void SplitContainer::DropOverlay::setRects(
    std::vector<SplitContainer::DropRect> _rects)
{
    this->rects_ = std::move(_rects);
}

// pajlada::Signals::NoArgSignal dragEnded;

void SplitContainer::DropOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    //            painter.fillRect(this->rect(), QColor("#334"));

    bool foundMover = false;

    for (DropRect &rect : this->rects_) {
        if (!foundMover && rect.rect.contains(this->mouseOverPoint_)) {
            painter.setBrush(getApp()->themes->splits.dropPreview);
            painter.setPen(getApp()->themes->splits.dropPreviewBorder);
            foundMover = true;
        } else {
            painter.setBrush(QColor(0, 0, 0, 0));
            painter.setPen(QColor(0, 0, 0, 0));
            // painter.setPen(getApp()->themes->splits.dropPreviewBorder);
        }

        painter.drawRect(rect.rect);
    }
}

void SplitContainer::DropOverlay::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void SplitContainer::DropOverlay::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();

    this->mouseOverPoint_ = event->pos();
    this->update();
}

void SplitContainer::DropOverlay::dragLeaveEvent(QDragLeaveEvent *)
{
    this->mouseOverPoint_ = QPoint(-10000, -10000);
    this->close();
    this->dragEnded.invoke();
}

void SplitContainer::DropOverlay::dropEvent(QDropEvent *event)
{
    Position *position = nullptr;
    for (DropRect &rect : this->rects_) {
        if (rect.rect.contains(this->mouseOverPoint_)) {
            position = &rect.position;
            break;
        }
    }

    if (position != nullptr) {
        this->parent_->insertSplit(SplitContainer::draggingSplit, *position);
        event->acceptProposedAction();
    }

    this->mouseOverPoint_ = QPoint(-10000, -10000);
    this->close();
    this->dragEnded.invoke();
}

//
// ResizeHandle
//

void SplitContainer::ResizeHandle::setVertical(bool isVertical)
{
    this->setCursor(isVertical ? Qt::SplitVCursor : Qt::SplitHCursor);
    this->vertical_ = isVertical;
}

SplitContainer::ResizeHandle::ResizeHandle(SplitContainer *_parent)
    : QWidget(_parent)
    , parent(_parent)
{
    this->setMouseTracking(true);
    this->hide();
}

void SplitContainer::ResizeHandle::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(QPen(getApp()->themes->splits.resizeHandle, 2));

    painter.fillRect(this->rect(),
                     getApp()->themes->splits.resizeHandleBackground);

    if (this->vertical_) {
        painter.drawLine(0, this->height() / 2, this->width(),
                         this->height() / 2);
    } else {
        painter.drawLine(this->width() / 2, 0, this->width() / 2,
                         this->height());
    }
}

void SplitContainer::ResizeHandle::mousePressEvent(QMouseEvent *event)
{
    this->isMouseDown_ = true;

    if (event->button() == Qt::RightButton) {
        this->resetFlex();
    }
}

void SplitContainer::ResizeHandle::mouseReleaseEvent(QMouseEvent *)
{
    this->isMouseDown_ = false;
}

void SplitContainer::ResizeHandle::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->isMouseDown_) {
        return;
    }

    assert(node != nullptr);
    assert(node->parent_ != nullptr);

    auto &siblings = node->parent_->getChildren();
    auto it = std::find_if(siblings.begin(), siblings.end(),
                           [this](const std::unique_ptr<Node> &n) {
                               return n.get() == this->node;
                           });

    assert(it != siblings.end());
    Node *before = siblings[it - siblings.begin() - 1].get();

    QPoint topLeft =
        this->parent->mapToGlobal(before->geometry_.topLeft().toPoint());
    QPoint bottomRight = this->parent->mapToGlobal(
        this->node->geometry_.bottomRight().toPoint());

    int globalX = topLeft.x() > event->globalX()
                      ? topLeft.x()
                      : (bottomRight.x() < event->globalX() ? bottomRight.x()
                                                            : event->globalX());
    int globalY = topLeft.y() > event->globalY()
                      ? topLeft.y()
                      : (bottomRight.y() < event->globalY() ? bottomRight.y()
                                                            : event->globalY());

    QPoint mousePoint(globalX, globalY);

    if (this->vertical_) {
        qreal totalFlexV = this->node->flexV_ + before->flexV_;
        before->flexV_ = totalFlexV * (mousePoint.y() - topLeft.y()) /
                         (bottomRight.y() - topLeft.y());
        this->node->flexV_ = totalFlexV - before->flexV_;

        this->parent->layout();

        // move handle
        this->move(this->x(), int(before->geometry_.bottom() - 4));
    } else {
        qreal totalFlexH = this->node->flexH_ + before->flexH_;
        before->flexH_ = totalFlexH * (mousePoint.x() - topLeft.x()) /
                         (bottomRight.x() - topLeft.x());
        this->node->flexH_ = totalFlexH - before->flexH_;

        this->parent->layout();

        // move handle
        this->move(int(before->geometry_.right() - 4), this->y());
    }
}

void SplitContainer::ResizeHandle::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();

    this->resetFlex();
}

void SplitContainer::ResizeHandle::resetFlex()
{
    for (auto &sibling : this->node->getParent()->getChildren()) {
        sibling->flexH_ = 1;
        sibling->flexV_ = 1;
    }

    this->parent->layout();
}

}  // namespace chatterino
