#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/helper/droppreview.hpp"
#include "widgets/helper/notebooktab.hpp"
#include "widgets/split.hpp"

#include <QDragEnterEvent>
#include <QHBoxLayout>
#include <QRect>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <vector>

#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

// remove
#include "application.hpp"
#include "singletons/thememanager.hpp"

namespace chatterino {
namespace widgets {

class SplitContainer : public BaseWidget, pajlada::Signals::SignalHolder
{
    Q_OBJECT

public:
    // fourtf: !!! preserve the order of left, up, right and down
    enum Direction { Left, Above, Right, Below };

    struct Node;

    struct Position {
    private:
        Position() = default;
        Position(Node *_relativeNode, Direction _direcion)
            : relativeNode(_relativeNode)
            , direction(_direcion)
        {
        }

        Node *relativeNode;
        Direction direction;

        friend struct Node;
        friend class SplitContainer;
    };

    struct DropRect {
        QRect rect;
        Position position;

        DropRect(const QRect &_rect, const Position &_position)
            : rect(_rect)
            , position(_position)
        {
        }
    };

    struct Node final {
        enum Type { EmptyRoot, _Split, VerticalContainer, HorizontalContainer };

        Type getType()
        {
            return this->type;
        }
        Split *getSplit()
        {
            return this->split;
        }
        const std::vector<std::unique_ptr<Node>> &getChildren()
        {
            return this->children;
        }

    private:
        Type type;
        Split *split;
        Node *parent;
        QRectF geometry;
        std::vector<std::unique_ptr<Node>> children;

        Node()
            : type(Type::EmptyRoot)
            , split(nullptr)
            , parent(nullptr)
        {
        }

        Node(Split *_split, Node *_parent)
            : type(Type::_Split)
            , split(_split)
            , parent(_parent)
        {
        }

        bool isOrContainsNode(Node *_node)
        {
            if (this == _node) {
                return true;
            }

            return std::any_of(
                this->children.begin(), this->children.end(),
                [_node](std::unique_ptr<Node> &n) { return n->isOrContainsNode(_node); });
        }

        Node *findNodeContainingSplit(Split *_split)
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

        void insertSplitRelative(Split *_split, Direction _direction)
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
                        if (toContainerType(_direction) == Node::HorizontalContainer) {
                            assert(false);
                        } else {
                            this->nestSplitIntoCollection(_split, _direction);
                        }
                    } break;
                    case Node::VerticalContainer: {
                        if (toContainerType(_direction) == Node::VerticalContainer) {
                            assert(false);
                        } else {
                            this->nestSplitIntoCollection(_split, _direction);
                        }
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

        void nestSplitIntoCollection(Split *_split, Direction _direction)
        {
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

        void _insertNextToThis(Split *_split, Direction _direction)
        {
            auto &siblings = this->parent->children;

            qreal width = this->parent->geometry.width() / siblings.size();
            qreal height = this->parent->geometry.height() / siblings.size();

            if (siblings.size() == 1) {
                this->geometry = QRect(0, 0, width, height);
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

        void setSplit(Split *_split)
        {
            assert(this->split == nullptr);
            assert(this->children.size() == 0);

            this->split = _split;
            this->type = Type::_Split;
        }

        Position releaseSplit()
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
                        position.direction =
                            siblings.begin() == it ? Direction::Above : Direction::Below;
                    } else {
                        position.direction =
                            siblings.begin() == it ? Direction::Left : Direction::Right;
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

        void layout(bool addSpacing, float _scale, std::vector<DropRect> &dropRects)
        {
            switch (this->type) {
                case Node::_Split: {
                    QRect rect = this->geometry.toRect();
                    this->split->setGeometry(rect.marginsRemoved(QMargins(1, 1, 1, 1)));
                } break;
                case Node::VerticalContainer: {
                    qreal totalHeight =
                        std::accumulate(this->children.begin(), this->children.end(), 0,
                                        [](qreal val, std::unique_ptr<Node> &node) {
                                            return val + node->geometry.height();
                                        });

                    qreal childX = this->geometry.left();
                    qreal childWidth = this->geometry.width();

                    if (addSpacing) {
                        qreal offset = std::min<qreal>(this->geometry.width() * 0.1, _scale * 24);

                        dropRects.emplace_back(
                            QRect(childX, this->geometry.top(), offset, this->geometry.height()),
                            Position(this, Direction::Left));
                        dropRects.emplace_back(
                            QRect(childX + this->geometry.width() - offset, this->geometry.top(),
                                  offset, this->geometry.height()),
                            Position(this, Direction::Right));

                        childX += offset;
                        childWidth -= offset * 2;
                    }

                    qreal scaleFactor = this->geometry.height() / totalHeight;

                    qreal y = this->geometry.top();
                    for (std::unique_ptr<Node> &child : this->children) {
                        child->geometry =
                            QRectF(childX, y, childWidth, child->geometry.height() * scaleFactor);

                        child->layout(addSpacing, _scale, dropRects);
                        y += child->geometry.height();
                    }
                } break;
                case Node::HorizontalContainer: {
                    qreal totalWidth =
                        std::accumulate(this->children.begin(), this->children.end(), 0,
                                        [](qreal val, std::unique_ptr<Node> &node) {
                                            return val + node->geometry.width();
                                        });

                    qreal childY = this->geometry.top();
                    qreal childHeight = this->geometry.height();

                    if (addSpacing) {
                        qreal offset = std::min<qreal>(this->geometry.height() * 0.1, _scale * 24);
                        dropRects.emplace_back(
                            QRect(this->geometry.left(), childY, this->geometry.width(), offset),
                            Position(this, Direction::Above));
                        dropRects.emplace_back(
                            QRect(this->geometry.left(), childY + this->geometry.height() - offset,
                                  this->geometry.width(), offset),
                            Position(this, Direction::Below));

                        childY += offset;
                        childHeight -= offset * 2;
                    }

                    qreal scaleFactor = this->geometry.width() / totalWidth;

                    qreal x = this->geometry.left();
                    for (std::unique_ptr<Node> &child : this->children) {
                        child->geometry =
                            QRectF(x, childY, child->geometry.width() * scaleFactor, childHeight);

                        child->layout(addSpacing, _scale, dropRects);
                        x += child->geometry.width();
                    }
                } break;
            };
        }

        static Type toContainerType(Direction _dir)
        {
            return _dir == Direction::Left || _dir == Direction::Right ? Type::HorizontalContainer
                                                                       : Type::VerticalContainer;
        }

        friend class SplitContainer;
    };

    class DropOverlay : public QWidget
    {
    public:
        DropOverlay(SplitContainer *_parent = nullptr)
            : QWidget(_parent)
            , parent(_parent)
            , mouseOverPoint(-10000, -10000)
        {
            this->setMouseTracking(true);
            this->setAcceptDrops(true);
        }

        void setRects(std::vector<SplitContainer::DropRect> _rects)
        {
            this->rects = std::move(_rects);
        }

        pajlada::Signals::NoArgSignal dragEnded;

    protected:
        void paintEvent(QPaintEvent *event) override
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

        void mouseMoveEvent(QMouseEvent *event)
        {
            this->mouseOverPoint = event->pos();
        }

        void leaveEvent(QEvent *event)
        {
            this->mouseOverPoint = QPoint(-10000, -10000);
        }

        void dragEnterEvent(QDragEnterEvent *event)
        {
            event->acceptProposedAction();
        }

        void dragMoveEvent(QDragMoveEvent *event)
        {
            event->acceptProposedAction();

            this->mouseOverPoint = event->pos();
            this->update();
        }

        void dragLeaveEvent(QDragLeaveEvent *event)
        {
            this->mouseOverPoint = QPoint(-10000, -10000);
            this->close();
            this->dragEnded.invoke();
        }

        void dropEvent(QDropEvent *event)
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

    private:
        std::vector<DropRect> rects;
        QPoint mouseOverPoint;
        SplitContainer *parent;
    };

    SplitContainer(Notebook *parent, NotebookTab *_tab);

    void appendNewSplit(bool openChannelNameDialog);
    void appendSplit(Split *split);
    void insertSplit(Split *split, const Position &position);
    void insertSplit(Split *split, Direction direction, Split *relativeTo);
    void insertSplit(Split *split, Direction direction, Node *relativeTo = nullptr);
    Position releaseSplit(Split *split);
    Position deleteSplit(Split *split);

    int getSplitCount()
    {
        return 0;
    }

    const std::vector<Split *> getSplits() const
    {
        return this->splits;
    }

    void refreshTabTitle();

    NotebookTab *getTab() const;
    const Node *getBaseNode()
    {
        return &this->baseNode;
    }

    static bool isDraggingSplit;
    static Split *draggingSplit;
    //    static Position dragOriginalPosition;

protected:
    //    bool eventFilter(QObject *object, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    //    void showEvent(QShowEvent *event) override;

    //    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

private:
    struct DropRegion {
        QRect rect;
        std::pair<int, int> position;

        DropRegion(QRect rect, std::pair<int, int> position)
        {
            this->rect = rect;
            this->position = position;
        }
    };

    std::vector<DropRect> dropRects;
    std::vector<DropRegion> dropRegions;
    NotebookPageDropPreview dropPreview;
    DropOverlay overlay;
    QPoint mouseOverPoint;

    void layout();

    Node baseNode;

    NotebookTab *tab;
    std::vector<Split *> splits;

    bool isDragging = false;

    //    struct {
    //        QVBoxLayout parentLayout;

    //        QHBoxLayout hbox;
    //    } ui;

    //    std::vector<Split *> splits;

    //    void setPreviewRect(QPoint mousePos);

    //    std::pair<int, int> getChatPosition(const Split *chatWidget);

    //    Split *createChatWidget();
};

}  // namespace widgets
}  // namespace chatterino
