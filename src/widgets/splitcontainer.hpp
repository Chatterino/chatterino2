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

namespace chatterino {
namespace widgets {

class SplitContainer : public BaseWidget
{
    Q_OBJECT

public:
    SplitContainer(Notebook *parent, NotebookTab *_tab);

    std::pair<int, int> removeFromLayout(Split *widget);
    void addToLayout(Split *widget, std::pair<int, int> position = std::pair<int, int>(-1, -1));

    const std::vector<Split *> &getSplits() const;
    std::vector<std::vector<Split *>> getColumns() const;
    NotebookTab *getTab() const;

    void addChat(bool openChannelNameDialog = false);

    static bool isDraggingSplit;
    static Split *draggingSplit;
    static std::pair<int, int> dropPosition;

    int currentX = 0;
    int currentY = 0;
    std::map<int, int> lastRequestedY;

    void refreshCurrentFocusCoordinates(bool alsoSetLastRequested = false);
    void requestFocus(int x, int y);

    void updateFlexValues();
    int splitCount() const;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

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

    NotebookTab *tab;

    struct {
        QVBoxLayout parentLayout;

        QHBoxLayout hbox;
    } ui;

    std::vector<Split *> splits;
    std::vector<DropRegion> dropRegions;

    NotebookPageDropPreview dropPreview;

    void setPreviewRect(QPoint mousePos);

    std::pair<int, int> getChatPosition(const Split *chatWidget);

    Split *createChatWidget();

public:
    void refreshTitle();

    void loadSplits();

    void save();
};

}  // namespace widgets
}  // namespace chatterino
