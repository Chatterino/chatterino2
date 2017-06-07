#pragma once

#include "widgets/chatwidget.h"
#include "widgets/notebookpage.h"
#include "widgets/notebookpagedroppreview.h"
#include "widgets/notebooktab.h"

#include <QDragEnterEvent>
#include <QHBoxLayout>
#include <QRect>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>
#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

class NotebookPage : public QWidget
{
    Q_OBJECT

public:
    NotebookPage(QWidget *parent, NotebookTab *_tab);

    std::pair<int, int> removeFromLayout(ChatWidget *widget);
    void addToLayout(ChatWidget *widget, std::pair<int, int> position);

    const std::vector<ChatWidget *> &getChatWidgets() const;
    NotebookTab *getTab() const;

    void addChat(bool openChannelNameDialog = false);

    static bool isDraggingSplit;
    static ChatWidget *draggingSplit;
    static std::pair<int, int> dropPosition;

protected:
    void paintEvent(QPaintEvent *) override;

    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
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

    NotebookTab *_tab;

    QVBoxLayout _parentbox;
    QHBoxLayout _hbox;

    std::vector<ChatWidget *> _chatWidgets;
    std::vector<DropRegion> _dropRegions;

    NotebookPageDropPreview _preview;

    void setPreviewRect(QPoint mousePos);

    std::pair<int, int> getChatPosition(const ChatWidget *chatWidget);

public:
    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
};

}  // namespace widgets
}  // namespace chatterino
