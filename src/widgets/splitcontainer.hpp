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
#include <boost/property_tree/ptree.hpp>
#include <boost/signals2.hpp>

namespace chatterino {

class ChannelManager;

namespace widgets {

class SplitContainer : public BaseWidget
{
    Q_OBJECT

public:
    SplitContainer(ChannelManager &_channelManager, Notebook *parent, NotebookTab *_tab);

    ChannelManager &channelManager;

    std::pair<int, int> removeFromLayout(Split *widget);
    void addToLayout(Split *widget, std::pair<int, int> position = std::pair<int, int>(-1, -1));

    const std::vector<Split *> &getChatWidgets() const;
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

protected:
    virtual bool eventFilter(QObject *object, QEvent *event) override;
    virtual void paintEvent(QPaintEvent *) override;

    virtual void showEvent(QShowEvent *) override;

    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;

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

    std::vector<Split *> chatWidgets;
    std::vector<DropRegion> dropRegions;

    NotebookPageDropPreview dropPreview;

    void setPreviewRect(QPoint mousePos);

    std::pair<int, int> getChatPosition(const Split *chatWidget);

    Split *createChatWidget();

public:
    void refreshTitle();

    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
};

}  // namespace widgets
}  // namespace chatterino
