#pragma once

#include "widgets/BaseWidget.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"

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

//
// Note: This class is a spaghetti container. There is a lot of spaghetti code
// inside but it doesn't expose any of it publicly.
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
        Position(Node *relativeNode, Direction direcion)
            : relativeNode_(relativeNode)
            , direction_(direcion)
        {
        }

        Node *relativeNode_;
        Direction direction_;

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
        Node();
        Node(Split *_split, Node *_parent);

        bool isOrContainsNode(Node *_node);
        Node *findNodeContainingSplit(Split *_split);
        void insertSplitRelative(Split *_split, Direction _direction);
        void nestSplitIntoCollection(Split *_split, Direction _direction);
        void insertNextToThis(Split *_split, Direction _direction);
        void setSplit(Split *_split);
        Position releaseSplit();
        qreal getFlex(bool isVertical);
        qreal getSize(bool isVertical);
        qreal getChildrensTotalFlex(bool isVertical);
        void layout(bool addSpacing, float _scale,
                    std::vector<DropRect> &dropRects_,
                    std::vector<ResizeRect> &resizeRects);

        static Type toContainerType(Direction _dir);

        Type type_;
        Split *split_;
        Node *preferedFocusTarget_;
        Node *parent_;
        QRectF geometry_;
        qreal flexH_ = 1;
        qreal flexV_ = 1;
        std::vector<std::unique_ptr<Node>> children_;

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
        std::vector<DropRect> rects_;
        QPoint mouseOverPoint_;
        SplitContainer *parent_;
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
        void mouseDoubleClickEvent(QMouseEvent *event) override;

        friend class SplitContainer;

    private:
        void resetFlex();

        bool vertical_;
        bool isMouseDown_ = false;
    };

public:
    SplitContainer(Notebook *parent);

    void appendNewSplit(bool openChannelNameDialog);
    void appendSplit(Split *split);
    void insertSplit(Split *split, const Position &position);
    void insertSplit(Split *split, Direction direction, Split *relativeTo);
    void insertSplit(Split *split, Direction direction,
                     Node *relativeTo = nullptr);
    Position releaseSplit(Split *split);
    Position deleteSplit(Split *split);

    void selectNextSplit(Direction direction);

    void decodeFromJson(QJsonObject &obj);

    int getSplitCount();
    const std::vector<Split *> getSplits() const;
    void refreshTabTitle();
    NotebookTab *getTab() const;
    Node *getBaseNode();

    void setTab(NotebookTab *tab_);
    void hideResizeHandles();
    void resetMouseStatus();

    static bool isDraggingSplit;
    static Split *draggingSplit;

protected:
    void paintEvent(QPaintEvent *event) override;

    void focusInEvent(QFocusEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

private:
    void layout();
    void setSelected(Split *selected_);
    void selectSplitRecursive(Node *node, Direction direction);
    void focusSplitRecursive(Node *node, Direction direction);
    void setPreferedTargetRecursive(Node *node);

    void addSplit(Split *split);

    void decodeNodeRecusively(QJsonObject &obj, Node *node);

    struct DropRegion {
        QRect rect;
        std::pair<int, int> position;

        DropRegion(QRect rect, std::pair<int, int> position)
        {
            this->rect = rect;
            this->position = position;
        }
    };

    std::vector<DropRect> dropRects_;
    std::vector<DropRegion> dropRegions_;
    DropOverlay overlay_;
    std::vector<std::unique_ptr<ResizeHandle>> resizeHandles_;
    QPoint mouseOverPoint_;

    Node baseNode_;
    Split *selected_;

    NotebookTab *tab_;
    std::vector<Split *> splits_;

    bool isDragging_ = false;
};

}  // namespace chatterino
