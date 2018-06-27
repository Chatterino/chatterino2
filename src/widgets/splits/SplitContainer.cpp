#include "widgets/splits/SplitContainer.hpp"
#include "Application.hpp"
#include "common/Common.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/ThemeManager.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Notebook.hpp"
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
#include <boost/foreach.hpp>

#include <algorithm>

namespace chatterino {

bool SplitContainer::isDraggingSplit = false;
Split *SplitContainer::draggingSplit = nullptr;

SplitContainer::SplitContainer(Notebook *parent)
    : BaseWidget(parent)
    , dropPreview(this)
    , overlay(this)
    , mouseOverPoint(-10000, -10000)
    , tab(nullptr)
{
    this->refreshTabTitle();

    this->managedConnect(Split::modifierStatusChanged, [this](auto modifiers) {
        this->layout();

        if (modifiers == showResizeHandlesModifiers) {
            for (std::unique_ptr<ResizeHandle> &handle : this->resizeHandles) {
                handle->show();
                handle->raise();
            }
        } else {
            for (std::unique_ptr<ResizeHandle> &handle : this->resizeHandles) {
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

void SplitContainer::setTab(NotebookTab *_tab)
{
    this->tab = _tab;

    this->tab->page = this;

    this->refreshTabTitle();
}

void SplitContainer::hideResizeHandles()
{
    this->overlay.hide();

    for (std::unique_ptr<ResizeHandle> &handle : this->resizeHandles) {
        handle->hide();
    }
}

void SplitContainer::resetMouseStatus()
{
    this->mouseOverPoint = QPoint(-10000, -10000);
    this->update();
}

void SplitContainer::appendNewSplit(bool openChannelNameDialog)
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
}

void SplitContainer::appendSplit(Split *split)
{
    this->insertSplit(split, Direction::Right);
}

void SplitContainer::insertSplit(Split *split, const Position &position)
{
    this->insertSplit(split, position.direction, reinterpret_cast<Node *>(position.relativeNode));
}

void SplitContainer::insertSplit(Split *split, Direction direction, Split *relativeTo)
{
    Node *node = this->baseNode.findNodeContainingSplit(relativeTo);
    assert(node != nullptr);

    this->insertSplit(split, direction, node);
}

void SplitContainer::insertSplit(Split *split, Direction direction, Node *relativeTo)
{
    assertInGuiThread();

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

    this->addSplit(split);
}

void SplitContainer::addSplit(Split *split)
{
    assertInGuiThread();

    split->setParent(this);
    split->show();
    split->giveFocus(Qt::MouseFocusReason);
    this->unsetCursor();
    this->splits.push_back(split);

    this->refreshTabTitle();

    split->getChannelView().tabHighlightRequested.connect([this](HighlightState state) {
        if (this->tab != nullptr) {
            this->tab->setHighlightState(state);
        }
    });

    split->focused.connect([this, split] { this->setSelected(split); });

    this->layout();
}

void SplitContainer::setSelected(Split *split)
{
    this->selected = split;

    if (Node *node = this->baseNode.findNodeContainingSplit(split)) {
        this->setPreferedTargetRecursive(node);
    }
}

void SplitContainer::setPreferedTargetRecursive(Node *node)
{
    if (node->parent != nullptr) {
        node->parent->preferedFocusTarget = node;

        this->setPreferedTargetRecursive(node->parent);
    }
}

SplitContainer::Position SplitContainer::releaseSplit(Split *split)
{
    assertInGuiThread();

    Node *node = this->baseNode.findNodeContainingSplit(split);
    assert(node != nullptr);

    this->splits.erase(std::find(this->splits.begin(), this->splits.end(), split));
    split->setParent(nullptr);
    Position position = node->releaseSplit();
    this->layout();
    if (splits.size() == 0) {
        this->setSelected(nullptr);
        this->setCursor(Qt::PointingHandCursor);
    } else {
        this->splits.front()->giveFocus(Qt::MouseFocusReason);
    }

    this->refreshTabTitle();

    // fourtf: really bad
    split->getChannelView().tabHighlightRequested.disconnectAll();

    split->getChannelView().tabHighlightRequested.disconnectAll();

    return position;
}

SplitContainer::Position SplitContainer::deleteSplit(Split *split)
{
    assertInGuiThread();
    assert(split != nullptr);

    split->deleteLater();
    return releaseSplit(split);
}

void SplitContainer::selectNextSplit(Direction direction)
{
    assertInGuiThread();

    if (Node *node = this->baseNode.findNodeContainingSplit(this->selected)) {
        this->selectSplitRecursive(node, direction);
    }
}

void SplitContainer::selectSplitRecursive(Node *node, Direction direction)
{
    if (node->parent != nullptr) {
        if (node->parent->type == Node::toContainerType(direction)) {
            auto &siblings = node->parent->children;

            auto it = std::find_if(siblings.begin(), siblings.end(),
                                   [node](const auto &other) { return other.get() == node; });
            assert(it != siblings.end());

            if (direction == Direction::Left || direction == Direction::Above) {
                if (it == siblings.begin()) {
                    this->selectSplitRecursive(node->parent, direction);
                } else {
                    this->focusSplitRecursive(siblings[it - siblings.begin() - 1].get(), direction);
                }
            } else {
                if (it->get() == siblings.back().get()) {
                    this->selectSplitRecursive(node->parent, direction);
                } else {
                    this->focusSplitRecursive(siblings[it - siblings.begin() + 1].get(), direction);
                }
            }
        } else {
            this->selectSplitRecursive(node->parent, direction);
        }
    }
}

void SplitContainer::focusSplitRecursive(Node *node, Direction direction)
{
    switch (node->type) {
        case Node::_Split: {
            node->split->giveFocus(Qt::OtherFocusReason);
        } break;

        case Node::HorizontalContainer:
        case Node::VerticalContainer: {
            auto &children = node->children;

            auto it = std::find_if(children.begin(), children.end(), [node](const auto &other) {
                return node->preferedFocusTarget == other.get();
            });

            if (it != children.end()) {
                this->focusSplitRecursive(it->get(), direction);
            } else {
                this->focusSplitRecursive(node->children.front().get(), direction);
            }
        } break;

        default:;
    }
}

void SplitContainer::layout()
{
    this->baseNode.geometry = this->rect();

    std::vector<DropRect> _dropRects;
    std::vector<ResizeRect> _resizeRects;
    this->baseNode.layout(Split::modifierStatus == showAddSplitRegions || this->isDragging,
                          this->getScale(), _dropRects, _resizeRects);

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

    // handle resizeHandles
    if (this->resizeHandles.size() < _resizeRects.size()) {
        while (this->resizeHandles.size() < _resizeRects.size()) {
            this->resizeHandles.push_back(std::make_unique<ResizeHandle>(this));
        }
    } else if (this->resizeHandles.size() > _resizeRects.size()) {
        this->resizeHandles.resize(_resizeRects.size());
    }

    {
        size_t i = 0;
        for (ResizeRect &resizeRect : _resizeRects) {
            ResizeHandle *handle = this->resizeHandles[i].get();
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
        if (this->splits.size() == 0) {
            // "Add Chat" was clicked
            this->appendNewSplit(true);
            this->mouseOverPoint = QPoint(-10000, -10000);

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

        QString text = "Click to add a split";

        Notebook *notebook = dynamic_cast<Notebook *>(this->parentWidget());

        if (notebook != nullptr) {
            if (notebook->getPageCount() > 1) {
                text += "\n\nTip: After adding a split you can hold <Alt> to move it or split it "
                        "further.";
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

    for (DropRect &dropRect : this->dropRects) {
        QColor border = getApp()->themes->splits.dropTargetRectBorder;
        QColor background = getApp()->themes->splits.dropTargetRect;

        if (!dropRect.rect.contains(this->mouseOverPoint)) {
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

        int s = std::min<int>(dropRect.rect.width(), dropRect.rect.height()) - 12;

        if (this->themeManager->isLightTheme()) {
            painter.setPen(QColor(0, 0, 0));
        } else {
            painter.setPen(QColor(255, 255, 255));
        }
        painter.drawLine(rect.left() + rect.width() / 2 - (s / 2), rect.top() + rect.height() / 2,
                         rect.left() + rect.width() / 2 + (s / 2), rect.top() + rect.height() / 2);
        painter.drawLine(rect.left() + rect.width() / 2, rect.top() + rect.height() / 2 - (s / 2),
                         rect.left() + rect.width() / 2, rect.top() + rect.height() / 2 + (s / 2));
    }

    QBrush accentColor = (QApplication::activeWindow() == this->window()
                              ? this->themeManager->tabs.selected.backgrounds.regular
                              : this->themeManager->tabs.selected.backgrounds.unfocused);

    painter.fillRect(0, 0, width(), 1, accentColor);
}

void SplitContainer::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
        return;

    if (!SplitContainer::isDraggingSplit)
        return;

    this->isDragging = true;
    this->layout();

    this->overlay.setGeometry(this->rect());
    this->overlay.show();
    this->overlay.raise();
}

void SplitContainer::mouseMoveEvent(QMouseEvent *event)
{
    if (Split::modifierStatus == showSplitOverlayModifiers) {
        this->setCursor(Qt::PointingHandCursor);
    }

    this->mouseOverPoint = event->pos();
    this->update();
}

void SplitContainer::leaveEvent(QEvent *)
{
    this->mouseOverPoint = QPoint(-10000, -10000);
    this->update();
}

void SplitContainer::focusInEvent(QFocusEvent *)
{
    if (this->baseNode.findNodeContainingSplit(this->selected) != nullptr) {
        this->selected->setFocus();
        return;
    }

    if (this->splits.size() != 0) {
        this->splits.front()->setFocus();
    }
}

void SplitContainer::refreshTabTitle()
{
    if (this->tab == nullptr) {
        return;
    }

    QString newTitle = "";
    bool first = true;

    for (const auto &chatWidget : this->splits) {
        auto channelName = chatWidget->getChannel()->name;
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

    this->tab->setDefaultTitle(newTitle);
}

int SplitContainer::getSplitCount()
{
    return 0;
}

const std::vector<Split *> SplitContainer::getSplits() const
{
    return this->splits;
}

SplitContainer::Node *SplitContainer::getBaseNode()
{
    return &this->baseNode;
}

void SplitContainer::decodeFromJson(QJsonObject &obj)
{
    assert(this->baseNode.type == Node::EmptyRoot);

    this->decodeNodeRecusively(obj, &this->baseNode);
}

void SplitContainer::decodeNodeRecusively(QJsonObject &obj, Node *node)
{
    QString type = obj.value("type").toString();

    if (type == "split") {
        auto *split = new Split(this);
        split->setChannel(chatterino::WindowManager::decodeChannel(obj.value("data").toObject()));

        this->appendSplit(split);
    } else if (type == "horizontal" || type == "vertical") {
        bool vertical = type == "vertical";

        Direction direction = vertical ? Direction::Below : Direction::Right;

        node->type = vertical ? Node::VerticalContainer : Node::HorizontalContainer;

        for (QJsonValue _val : obj.value("items").toArray()) {
            auto _obj = _val.toObject();

            auto _type = _obj.value("type");
            if (_type == "split") {
                auto *split = new Split(this);
                split->setChannel(
                    chatterino::WindowManager::decodeChannel(_obj.value("data").toObject()));

                Node *_node = new Node();
                _node->parent = node;
                _node->split = split;
                _node->type = Node::_Split;

                _node->flexH = _obj.value("flexh").toDouble(1.0);
                _node->flexV = _obj.value("flexv").toDouble(1.0);
                node->children.emplace_back(_node);

                this->addSplit(split);
            } else {
                Node *_node = new Node();
                _node->parent = node;
                node->children.emplace_back(_node);
                this->decodeNodeRecusively(_obj, _node);
            }
        }

        for (int i = 0; i < 2; i++) {
            if (node->getChildren().size() < 2) {
                auto *split = new Split(this);
                split->setChannel(
                    chatterino::WindowManager::decodeChannel(obj.value("data").toObject()));

                this->insertSplit(split, direction, node);
            }
        }
    }
}

//
// Node
//

SplitContainer::Node::Type SplitContainer::Node::getType()
{
    return this->type;
}
Split *SplitContainer::Node::getSplit()
{
    return this->split;
}

SplitContainer::Node *SplitContainer::Node::getParent()
{
    return this->parent;
}

qreal SplitContainer::Node::getHorizontalFlex()
{
    return this->flexH;
}

qreal SplitContainer::Node::getVerticalFlex()
{
    return this->flexV;
}

const std::vector<std::unique_ptr<SplitContainer::Node>> &SplitContainer::Node::getChildren()
{
    return this->children;
}

SplitContainer::Node::Node()
    : type(SplitContainer::Node::Type::EmptyRoot)
    , split(nullptr)
    , parent(nullptr)
{
}

SplitContainer::Node::Node(Split *_split, Node *_parent)
    : type(Type::_Split)
    , split(_split)
    , parent(_parent)
{
}

bool SplitContainer::Node::isOrContainsNode(SplitContainer::Node *_node)
{
    if (this == _node) {
        return true;
    }

    return std::any_of(this->children.begin(), this->children.end(),
                       [_node](std::unique_ptr<Node> &n) { return n->isOrContainsNode(_node); });
}

SplitContainer::Node *SplitContainer::Node::findNodeContainingSplit(Split *_split)
{
    if (this->type == Type::_Split && this->split == _split) {
        return this;
    }

    for (std::unique_ptr<Node> &node : this->children) {
        Node *a = node->findNodeContainingSplit(_split);

        if (a != nullptr) {
            return a;
        }
    }
    return nullptr;
}

void SplitContainer::Node::insertSplitRelative(Split *_split, Direction _direction)
{
    if (this->parent == nullptr) {
        switch (this->type) {
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
    if (parent->type == toContainerType(_direction)) {
        // hell yeah we'll just insert it next to outselves
        this->_insertNextToThis(_split, _direction);
    } else {
        this->nestSplitIntoCollection(_split, _direction);
    }
}

void SplitContainer::Node::nestSplitIntoCollection(Split *_split, Direction _direction)
{
    if (toContainerType(_direction) == this->type) {
        this->children.emplace_back(new Node(_split, this));
    } else {
        // we'll need to nest outselves
        // move all our data into a new node
        Node *clone = new Node();
        clone->type = this->type;
        clone->children = std::move(this->children);
        for (std::unique_ptr<Node> &node : clone->children) {
            node->parent = clone;
        }
        clone->split = this->split;
        clone->parent = this;

        // add the node to our children and change our type
        this->children.push_back(std::unique_ptr<Node>(clone));
        this->type = toContainerType(_direction);
        this->split = nullptr;

        clone->_insertNextToThis(_split, _direction);
    }
}

void SplitContainer::Node::_insertNextToThis(Split *_split, Direction _direction)
{
    auto &siblings = this->parent->children;

    qreal width = this->parent->geometry.width() / siblings.size();
    qreal height = this->parent->geometry.height() / siblings.size();

    if (siblings.size() == 1) {
        this->geometry = QRect(0, 0, int(width), int(height));
    }

    auto it = std::find_if(siblings.begin(), siblings.end(),
                           [this](auto &node) { return this == node.get(); });

    assert(it != siblings.end());
    if (_direction == Direction::Right || _direction == Direction::Below) {
        it++;
    }

    Node *node = new Node(_split, this->parent);
    node->geometry = QRectF(0, 0, width, height);
    siblings.insert(it, std::unique_ptr<Node>(node));
}

void SplitContainer::Node::setSplit(Split *_split)
{
    assert(this->split == nullptr);
    assert(this->children.size() == 0);

    this->split = _split;
    this->type = Type::_Split;
}

SplitContainer::Position SplitContainer::Node::releaseSplit()
{
    assert(this->type == Type::_Split);

    if (parent == nullptr) {
        this->type = Type::EmptyRoot;
        this->split = nullptr;

        Position pos;
        pos.relativeNode = nullptr;
        pos.direction = Direction::Right;
        return pos;
    } else {
        auto &siblings = this->parent->children;

        auto it = std::find_if(begin(siblings), end(siblings),
                               [this](auto &node) { return this == node.get(); });
        assert(it != siblings.end());

        Position position;
        if (siblings.size() == 2) {
            // delete this and move split to parent
            position.relativeNode = this->parent;
            if (this->parent->type == Type::VerticalContainer) {
                position.direction = siblings.begin() == it ? Direction::Above : Direction::Below;
            } else {
                position.direction = siblings.begin() == it ? Direction::Left : Direction::Right;
            }

            Node *_parent = this->parent;
            siblings.erase(it);
            std::unique_ptr<Node> &sibling = siblings.front();
            _parent->type = sibling->type;
            _parent->split = sibling->split;
            std::vector<std::unique_ptr<Node>> nodes = std::move(sibling->children);
            for (auto &node : nodes) {
                node->parent = _parent;
            }
            _parent->children = std::move(nodes);
        } else {
            if (this == siblings.back().get()) {
                position.direction = this->parent->type == Type::VerticalContainer
                                         ? Direction::Below
                                         : Direction::Right;
                siblings.erase(it);
                position.relativeNode = siblings.back().get();
            } else {
                position.relativeNode = (it + 1)->get();
                position.direction = this->parent->type == Type::VerticalContainer
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
    return isVertical ? this->flexV : this->flexH;
}

qreal SplitContainer::Node::getSize(bool isVertical)
{
    return isVertical ? this->geometry.height() : this->geometry.width();
}

qreal SplitContainer::Node::getChildrensTotalFlex(bool isVertical)
{
    return std::accumulate(
        this->children.begin(), this->children.end(), qreal(0),
        [=](qreal val, std::unique_ptr<Node> &node) { return val + node->getFlex(isVertical); });
}

void SplitContainer::Node::layout(bool addSpacing, float _scale, std::vector<DropRect> &dropRects,
                                  std::vector<ResizeRect> &resizeRects)
{
    for (std::unique_ptr<Node> &node : this->children) {
        if (node->flexH <= 0)
            node->flexH = 0;
        if (node->flexV <= 0)
            node->flexV = 0;
    }

    switch (this->type) {
        case Node::_Split: {
            QRect rect = this->geometry.toRect();
            this->split->setGeometry(rect.marginsRemoved(QMargins(1, 1, 1, 1)));
        } break;
        case Node::VerticalContainer:
        case Node::HorizontalContainer: {
            bool isVertical = this->type == Node::VerticalContainer;

            // vars
            qreal minSize = qreal(48 * _scale);

            qreal totalFlex = this->getChildrensTotalFlex(isVertical);
            qreal totalSize = std::accumulate(
                this->children.begin(), this->children.end(), qreal(0),
                [=](int val, std::unique_ptr<Node> &node) {
                    return val + std::max<qreal>(this->getSize(isVertical) / totalFlex *
                                                     node->getFlex(isVertical),
                                                 minSize);
                });

            qreal sizeMultiplier = this->getSize(isVertical) / totalSize;
            QRectF childRect = this->geometry;

            // add spacing if reqested
            if (addSpacing) {
                qreal offset =
                    std::min<qreal>(this->getSize(!isVertical) * 0.1, qreal(_scale * 24));

                // droprect left / above
                dropRects.emplace_back(
                    QRectF(this->geometry.left(), this->geometry.top(),
                           isVertical ? offset : this->geometry.width(),
                           isVertical ? this->geometry.height() : offset)
                        .toRect(),
                    Position(this, isVertical ? Direction::Left : Direction::Above));

                // droprect right / below
                if (isVertical) {
                    dropRects.emplace_back(
                        QRectF(this->geometry.right() - offset, this->geometry.top(), offset,
                               this->geometry.height())
                            .toRect(),
                        Position(this, Direction::Right));
                } else {
                    dropRects.emplace_back(
                        QRectF(this->geometry.left(), this->geometry.bottom() - offset,
                               this->geometry.width(), offset)
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
            qreal pos = isVertical ? childRect.top() : childRect.left();
            for (std::unique_ptr<Node> &child : this->children) {
                // set rect
                QRectF rect = childRect;
                if (isVertical) {
                    rect.setTop(pos);
                    rect.setHeight(
                        std::max<qreal>(this->geometry.height() / totalFlex * child->flexV,
                                        minSize) *
                        sizeMultiplier);
                } else {
                    rect.setLeft(pos);
                    rect.setWidth(std::max<qreal>(this->geometry.width() / totalFlex * child->flexH,
                                                  minSize) *
                                  sizeMultiplier);
                }

                child->geometry = rect;
                child->layout(addSpacing, _scale, dropRects, resizeRects);

                pos += child->getSize(isVertical);

                // add resize rect
                if (child != this->children.front()) {
                    QRectF r = isVertical ? QRectF(this->geometry.left(), child->geometry.top() - 4,
                                                   this->geometry.width(), 8)
                                          : QRectF(child->geometry.left() - 4, this->geometry.top(),
                                                   8, this->geometry.height());
                    resizeRects.push_back(ResizeRect(r.toRect(), child.get(), isVertical));
                }

                // normalize flex
                if (isVertical) {
                    child->flexV = child->flexV / totalFlex * this->children.size();
                    child->flexH = 1;
                } else {
                    child->flexH = child->flexH / totalFlex * this->children.size();
                    child->flexV = 1;
                }
            }
        } break;
    };
}

SplitContainer::Node::Type SplitContainer::Node::toContainerType(Direction _dir)
{
    return _dir == Direction::Left || _dir == Direction::Right ? Type::HorizontalContainer
                                                               : Type::VerticalContainer;
}

//
// DropOverlay
//

SplitContainer::DropOverlay::DropOverlay(SplitContainer *_parent)
    : QWidget(_parent)
    , mouseOverPoint(-10000, -10000)
    , parent(_parent)
{
    this->setMouseTracking(true);
    this->setAcceptDrops(true);
}

void SplitContainer::DropOverlay::setRects(std::vector<SplitContainer::DropRect> _rects)
{
    this->rects = std::move(_rects);
}

// pajlada::Signals::NoArgSignal dragEnded;

void SplitContainer::DropOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    //            painter.fillRect(this->rect(), QColor("#334"));

    bool foundMover = false;

    for (DropRect &rect : this->rects) {
        if (!foundMover && rect.rect.contains(this->mouseOverPoint)) {
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

    this->mouseOverPoint = event->pos();
    this->update();
}

void SplitContainer::DropOverlay::dragLeaveEvent(QDragLeaveEvent *)
{
    this->mouseOverPoint = QPoint(-10000, -10000);
    this->close();
    this->dragEnded.invoke();
}

void SplitContainer::DropOverlay::dropEvent(QDropEvent *event)
{
    Position *position = nullptr;
    for (DropRect &rect : this->rects) {
        if (rect.rect.contains(this->mouseOverPoint)) {
            position = &rect.position;
            break;
        }
    }

    if (position != nullptr) {
        this->parent->insertSplit(SplitContainer::draggingSplit, *position);
        event->acceptProposedAction();
    }

    this->mouseOverPoint = QPoint(-10000, -10000);
    this->close();
    this->dragEnded.invoke();
}

//
// ResizeHandle
//

void SplitContainer::ResizeHandle::setVertical(bool isVertical)
{
    this->setCursor(isVertical ? Qt::SplitVCursor : Qt::SplitHCursor);
    this->vertical = isVertical;
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

    painter.fillRect(this->rect(), getApp()->themes->splits.resizeHandleBackground);

    if (this->vertical) {
        painter.drawLine(0, this->height() / 2, this->width(), this->height() / 2);
    } else {
        painter.drawLine(this->width() / 2, 0, this->width() / 2, this->height());
    }
}

void SplitContainer::ResizeHandle::mousePressEvent(QMouseEvent *)
{
    this->isMouseDown = true;
}

void SplitContainer::ResizeHandle::mouseReleaseEvent(QMouseEvent *)
{
    this->isMouseDown = false;
}

void SplitContainer::ResizeHandle::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->isMouseDown) {
        return;
    }

    assert(node != nullptr);
    assert(node->parent != nullptr);

    auto &siblings = node->parent->getChildren();
    auto it =
        std::find_if(siblings.begin(), siblings.end(),
                     [this](const std::unique_ptr<Node> &n) { return n.get() == this->node; });

    assert(it != siblings.end());
    Node *before = siblings[it - siblings.begin() - 1].get();

    QPoint topLeft = this->parent->mapToGlobal(before->geometry.topLeft().toPoint());
    QPoint bottomRight = this->parent->mapToGlobal(this->node->geometry.bottomRight().toPoint());

    int globalX = topLeft.x() > event->globalX()
                      ? topLeft.x()
                      : (bottomRight.x() < event->globalX() ? bottomRight.x() : event->globalX());
    int globalY = topLeft.y() > event->globalY()
                      ? topLeft.y()
                      : (bottomRight.y() < event->globalY() ? bottomRight.y() : event->globalY());

    QPoint mousePoint(globalX, globalY);

    if (this->vertical) {
        qreal totalFlexV = this->node->flexV + before->flexV;
        before->flexV =
            totalFlexV * (mousePoint.y() - topLeft.y()) / (bottomRight.y() - topLeft.y());
        this->node->flexV = totalFlexV - before->flexV;

        this->parent->layout();

        // move handle
        this->move(this->x(), int(before->geometry.bottom() - 4));
    } else {
        qreal totalFlexH = this->node->flexH + before->flexH;
        before->flexH =
            totalFlexH * (mousePoint.x() - topLeft.x()) / (bottomRight.x() - topLeft.x());
        this->node->flexH = totalFlexH - before->flexH;

        this->parent->layout();

        // move handle
        this->move(int(before->geometry.right() - 4), this->y());
    }
}

}  // namespace chatterino
