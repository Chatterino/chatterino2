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

class QJsonObject;

namespace chatterino {
namespace widgets {

//
// Note: This class is a spaghetti container. There is a lot of spaghetti code inside but it doesn't
// expose any of it publicly.
//

class SplitContainer : public BaseWidget, pajlada::Signals::SignalHolder
{
    Q_OBJECT

public:
    struct Node;

    // fourtf: !!! preserve the order of left, up, right and down
    enum Direction { Left, Above, Right, Below };

    struct Position final {
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

private:
    struct DropRect final {
        QRect rect;
        Position position;

        DropRect(const QRect &_rect, const Position &_position)
            : rect(_rect)
            , position(_position)
        {
        }
    };

    struct ResizeRect final {
        QRect rect;
        Node *node;
        bool vertical;

        ResizeRect(const QRect &_rect, Node *_node, bool _vertical)
            : rect(_rect)
            , node(_node)
            , vertical(_vertical)
        {
        }
    };

public:
    struct Node final {
        enum Type { EmptyRoot, _Split, VerticalContainer, HorizontalContainer };

        Type getType();
        Split *getSplit();
        Node *getParent();
        qreal getHorizontalFlex();
        qreal getVerticalFlex();
        const std::vector<std::unique_ptr<Node>> &getChildren();

    private:
        Type type;
        Split *split;
        Node *parent;
        QRectF geometry;
        qreal flexH = 1;
        qreal flexV = 1;
        std::vector<std::unique_ptr<Node>> children;

        Node();
        Node(Split *_split, Node *_parent);

        bool isOrContainsNode(Node *_node);
        Node *findNodeContainingSplit(Split *_split);
        void insertSplitRelative(Split *_split, Direction _direction);
        void nestSplitIntoCollection(Split *_split, Direction _direction);
        void _insertNextToThis(Split *_split, Direction _direction);
        void setSplit(Split *_split);
        Position releaseSplit();
        qreal getFlex(bool isVertical);
        qreal getSize(bool isVertical);
        qreal getChildrensTotalFlex(bool isVertical);
        void layout(bool addSpacing, float _scale, std::vector<DropRect> &dropRects,
                    std::vector<ResizeRect> &resizeRects);

        static Type toContainerType(Direction _dir);

        friend class SplitContainer;
    };

private:
    class DropOverlay final : public QWidget
    {
    public:
        DropOverlay(SplitContainer *_parent = nullptr);

        void setRects(std::vector<SplitContainer::DropRect> _rects);

        pajlada::Signals::NoArgSignal dragEnded;

    protected:
        void paintEvent(QPaintEvent *event) override;
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dragMoveEvent(QDragMoveEvent *event) override;
        void dragLeaveEvent(QDragLeaveEvent *event) override;
        void dropEvent(QDropEvent *event) override;

    private:
        std::vector<DropRect> rects;
        QPoint mouseOverPoint;
        SplitContainer *parent;
    };

    class ResizeHandle final : public QWidget
    {
    public:
        SplitContainer *parent;
        Node *node;

        void setVertical(bool isVertical);
        ResizeHandle(SplitContainer *_parent = nullptr);
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;

        friend class SplitContainer;

    private:
        bool vertical;
        bool isMouseDown = false;
    };

public:
    SplitContainer(Notebook *parent);

    void appendNewSplit(bool openChannelNameDialog);
    void appendSplit(Split *split);
    void insertSplit(Split *split, const Position &position);
    void insertSplit(Split *split, Direction direction, Split *relativeTo);
    void insertSplit(Split *split, Direction direction, Node *relativeTo = nullptr);
    Position releaseSplit(Split *split);
    Position deleteSplit(Split *split);

    void decodeFromJson(QJsonObject &obj);

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
    Node *getBaseNode()
    {
        return &this->baseNode;
    }

    void setTab(NotebookTab *tab);

    static bool isDraggingSplit;
    static Split *draggingSplit;

protected:
    void paintEvent(QPaintEvent *event) override;

    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

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

    void addSplit(Split *split);

    std::vector<DropRect> dropRects;
    std::vector<DropRegion> dropRegions;
    NotebookPageDropPreview dropPreview;
    DropOverlay overlay;
    std::vector<std::unique_ptr<ResizeHandle>> resizeHandles;
    QPoint mouseOverPoint;

    void layout();

    Node baseNode;

    NotebookTab *tab;
    std::vector<Split *> splits;

    bool isDragging = false;

    void decodeNodeRecusively(QJsonObject &obj, Node *node);
};

}  // namespace widgets
}  // namespace chatterino
