#ifndef NOTEBOOKPAGE_H
#define NOTEBOOKPAGE_H

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

namespace chatterino {
namespace widgets {

class NotebookPage : public QWidget
{
    Q_OBJECT

public:
    NotebookPage(QWidget *parent, NotebookTab *tab);
    NotebookTab *tab;

    std::pair<int, int> removeFromLayout(ChatWidget *widget);
    void addToLayout(ChatWidget *widget, std::pair<int, int> position);

    const std::vector<ChatWidget *> &
    getChatWidgets() const
    {
        return chatWidgets;
    }

    static bool isDraggingSplit;
    static ChatWidget *draggingSplit;
    static std::pair<int, int> dropPosition;

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

    struct DropRegion {
        QRect rect;
        std::pair<int, int> position;

        DropRegion(QRect rect, std::pair<int, int> position)
        {
            this->rect = rect;
            this->position = position;
        }
    };

    QVBoxLayout parentbox;
    QHBoxLayout hbox;

    std::vector<ChatWidget *> chatWidgets;
    std::vector<DropRegion> dropRegions;

    NotebookPageDropPreview preview;

private:
    void setPreviewRect(QPoint mousePos);

public:
    void load(const boost::property_tree::ptree &v);
};

}  // namespace widgets
}  // namespace chatterino

#endif  // NOTEBOOKPAGE_H
