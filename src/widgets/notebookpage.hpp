#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/chatwidget.hpp"
#include "widgets/notebookpage.hpp"
#include "widgets/notebookpagedroppreview.hpp"
#include "widgets/notebooktab.hpp"

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
class CompletionManager;

namespace widgets {

class NotebookPage : public BaseWidget
{
    Q_OBJECT

public:
    NotebookPage(ChannelManager &_channelManager, Notebook *parent, NotebookTab *_tab);

    ChannelManager &channelManager;
    CompletionManager &completionManager;

    std::pair<int, int> removeFromLayout(ChatWidget *widget);
    void addToLayout(ChatWidget *widget, std::pair<int, int> position);

    const std::vector<ChatWidget *> &getChatWidgets() const;
    NotebookTab *getTab() const;

    void addChat(bool openChannelNameDialog = false);

    static bool isDraggingSplit;
    static ChatWidget *draggingSplit;
    static std::pair<int, int> dropPosition;

    int currentX = 0;
    int currentY = 0;
    std::map<int, int> lastRequestedY;

    void refreshCurrentFocusCoordinates(bool alsoSetLastRequested = false);
    void requestFocus(int x, int y);

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

    std::vector<ChatWidget *> chatWidgets;
    std::vector<DropRegion> dropRegions;

    NotebookPageDropPreview dropPreview;

    void setPreviewRect(QPoint mousePos);

    std::pair<int, int> getChatPosition(const ChatWidget *chatWidget);

    ChatWidget *createChatWidget();

public:
    void refreshTitle();

    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
};

}  // namespace widgets
}  // namespace chatterino
