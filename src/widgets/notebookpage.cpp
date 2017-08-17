#include "widgets/notebookpage.hpp"
#include "colorscheme.hpp"
#include "widgets/chatwidget.hpp"
#include "widgets/notebook.hpp"
#include "widgets/notebooktab.hpp"

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QMimeData>
#include <QObject>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/foreach.hpp>

#include <algorithm>

namespace chatterino {
namespace widgets {

bool NotebookPage::isDraggingSplit = false;
ChatWidget *NotebookPage::draggingSplit = nullptr;
std::pair<int, int> NotebookPage::dropPosition = std::pair<int, int>(-1, -1);

NotebookPage::NotebookPage(ChannelManager &_channelManager, Notebook *parent, NotebookTab *_tab)
    : BaseWidget(parent->colorScheme, parent)
    , channelManager(_channelManager)
    , completionManager(parent->completionManager)
    , tab(_tab)
    , dropPreview(this)
{
    this->tab->page = this;

    this->setLayout(&this->ui.parentLayout);

    this->setHidden(true);
    this->setAcceptDrops(true);

    this->ui.parentLayout.addSpacing(2);
    this->ui.parentLayout.addLayout(&this->ui.hbox);
    this->ui.parentLayout.setMargin(0);

    this->ui.hbox.setSpacing(1);
    this->ui.hbox.setMargin(0);

    this->refreshTitle();
}

std::pair<int, int> NotebookPage::removeFromLayout(ChatWidget *widget)
{
    // remove reference to chat widget from chatWidgets vector
    auto it = std::find(std::begin(this->chatWidgets), std::end(this->chatWidgets), widget);
    if (it != std::end(this->chatWidgets)) {
        this->chatWidgets.erase(it);

        this->refreshTitle();
    }

    // remove from box and return location
    for (int i = 0; i < this->ui.hbox.count(); ++i) {
        auto vbox = static_cast<QVBoxLayout *>(this->ui.hbox.itemAt(i));

        for (int j = 0; j < vbox->count(); ++j) {
            if (vbox->itemAt(j)->widget() != widget) {
                continue;
            }

            widget->setParent(nullptr);

            bool isLastItem = vbox->count() == 0;

            if (isLastItem) {
                this->ui.hbox.removeItem(vbox);

                delete vbox;
            }

            return std::pair<int, int>(i, isLastItem ? -1 : j);
        }
    }

    return std::pair<int, int>(-1, -1);
}

void NotebookPage::addToLayout(ChatWidget *widget,
                               std::pair<int, int> position = std::pair<int, int>(-1, -1))
{
    this->chatWidgets.push_back(widget);

    this->refreshTitle();

    widget->giveFocus(Qt::MouseFocusReason);

    // add vbox at the end
    if (position.first < 0 || position.first >= this->ui.hbox.count()) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        this->ui.hbox.addLayout(vbox, 1);
        this->refreshCurrentFocusCoordinates();
        return;
    }

    // insert vbox
    if (position.second == -1) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        this->ui.hbox.insertLayout(position.first, vbox, 1);
        this->refreshCurrentFocusCoordinates();
        return;
    }

    // add to existing vbox
    auto vbox = static_cast<QVBoxLayout *>(this->ui.hbox.itemAt(position.first));

    vbox->insertWidget(std::max(0, std::min(vbox->count(), position.second)), widget);

    this->refreshCurrentFocusCoordinates();
}

const std::vector<ChatWidget *> &NotebookPage::getChatWidgets() const
{
    return this->chatWidgets;
}

NotebookTab *NotebookPage::getTab() const
{
    return this->tab;
}

void NotebookPage::addChat(bool openChannelNameDialog)
{
    ChatWidget *w = this->createChatWidget();

    if (openChannelNameDialog) {
        bool ret = w->showChangeChannelPopup("Open channel", true);

        if (!ret) {
            delete w;
            return;
        }
    }

    this->addToLayout(w, std::pair<int, int>(-1, -1));
}

void NotebookPage::refreshCurrentFocusCoordinates(bool alsoSetLastRequested)
{
    int setX = -1;
    int setY = -1;
    bool doBreak = false;
    for (int x = 0; x < this->ui.hbox.count(); ++x) {
        QLayoutItem *item = this->ui.hbox.itemAt(x);
        if (item->isEmpty()) {
            setX = x;
            break;
        }
        QVBoxLayout *vbox = static_cast<QVBoxLayout *>(item->layout());

        for (int y = 0; y < vbox->count(); ++y) {
            QLayoutItem *innerItem = vbox->itemAt(y);

            if (innerItem->isEmpty()) {
                setX = x;
                setY = y;
                doBreak = true;
                break;
            }

            QWidget *w = innerItem->widget();
            if (w) {
                ChatWidget *chatWidget = static_cast<ChatWidget *>(w);
                if (chatWidget->hasFocus()) {
                    setX = x;
                    setY = y;
                    doBreak = true;
                    break;
                }
            }
        }

        if (doBreak) {
            break;
        }
    }

    if (setX != -1) {
        this->currentX = setX;

        if (setY != -1) {
            this->currentY = setY;

            if (alsoSetLastRequested) {
                this->lastRequestedY[setX] = setY;
            }
        }
    }
}

void NotebookPage::requestFocus(int requestedX, int requestedY)
{
    // XXX: Perhaps if we request an Y coordinate out of bounds, we shuold set all previously set
    // requestedYs to 0 (if -1 is requested) or that x-coordinates vbox count (if requestedY >=
    // currentvbox.count() is requested)
    if (requestedX < 0 || requestedX >= this->ui.hbox.count()) {
        return;
    }

    QLayoutItem *item = this->ui.hbox.itemAt(requestedX);
    QWidget *xW = item->widget();
    if (item->isEmpty()) {
        qDebug() << "Requested hbox item " << requestedX << "is empty";
        if (xW) {
            qDebug() << "but xW is not null";
            // TODO: figure out what to do here
        }
        return;
    }

    QVBoxLayout *vbox = static_cast<QVBoxLayout *>(item->layout());

    if (requestedY < 0) {
        requestedY = 0;
    } else if (requestedY >= vbox->count()) {
        requestedY = vbox->count() - 1;
    }

    this->lastRequestedY[requestedX] = requestedY;

    QLayoutItem *innerItem = vbox->itemAt(requestedY);

    if (innerItem->isEmpty()) {
        qDebug() << "Requested vbox item " << requestedY << "is empty";
        return;
    }

    QWidget *w = innerItem->widget();
    if (w) {
        ChatWidget *chatWidget = static_cast<ChatWidget *>(w);
        chatWidget->giveFocus(Qt::OtherFocusReason);
    }
}

void NotebookPage::enterEvent(QEvent *)
{
    if (this->ui.hbox.count() == 0) {
        this->setCursor(QCursor(Qt::PointingHandCursor));
    } else {
        this->setCursor(QCursor(Qt::ArrowCursor));
    }
}

void NotebookPage::leaveEvent(QEvent *)
{
}

void NotebookPage::mouseReleaseEvent(QMouseEvent *event)
{
    if (this->ui.hbox.count() == 0 && event->button() == Qt::LeftButton) {
        // "Add Chat" was clicked
        this->addChat(true);

        this->setCursor(QCursor(Qt::ArrowCursor));
    }
}

void NotebookPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
        return;

    if (!isDraggingSplit) {
        return;
    }

    this->dropRegions.clear();

    if (this->ui.hbox.count() == 0) {
        this->dropRegions.push_back(DropRegion(rect(), std::pair<int, int>(-1, -1)));
    } else {
        for (int i = 0; i < this->ui.hbox.count() + 1; ++i) {
            this->dropRegions.push_back(
                DropRegion(QRect(((i * 4 - 1) * width() / this->ui.hbox.count()) / 4, 0,
                                 width() / this->ui.hbox.count() / 2 + 1, height() + 1),
                           std::pair<int, int>(i, -1)));
        }

        for (int i = 0; i < this->ui.hbox.count(); ++i) {
            auto vbox = static_cast<QVBoxLayout *>(this->ui.hbox.itemAt(i));

            for (int j = 0; j < vbox->count() + 1; ++j) {
                this->dropRegions.push_back(DropRegion(
                    QRect(i * width() / this->ui.hbox.count(),
                          ((j * 2 - 1) * height() / vbox->count()) / 2,
                          width() / this->ui.hbox.count() + 1, height() / vbox->count() + 1),

                    std::pair<int, int>(i, j)));
            }
        }
    }

    setPreviewRect(event->pos());

    event->acceptProposedAction();
}

void NotebookPage::dragMoveEvent(QDragMoveEvent *event)
{
    setPreviewRect(event->pos());
}

void NotebookPage::setPreviewRect(QPoint mousePos)
{
    for (DropRegion region : this->dropRegions) {
        if (region.rect.contains(mousePos)) {
            this->dropPreview.setBounds(region.rect);

            if (!this->dropPreview.isVisible()) {
                this->dropPreview.show();
                this->dropPreview.raise();
            }

            dropPosition = region.position;

            return;
        }
    }

    this->dropPreview.hide();
}

void NotebookPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    this->dropPreview.hide();
}

void NotebookPage::dropEvent(QDropEvent *event)
{
    if (isDraggingSplit) {
        event->acceptProposedAction();

        NotebookPage::draggingSplit->setParent(this);

        addToLayout(NotebookPage::draggingSplit, dropPosition);
    }

    this->dropPreview.hide();
}

bool NotebookPage::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);

        this->refreshCurrentFocusCoordinates((focusEvent->reason() == Qt::MouseFocusReason));
    }

    return false;
}

void NotebookPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (this->ui.hbox.count() == 0) {
        painter.fillRect(rect(), this->colorScheme.ChatBackground);

        painter.setPen(this->colorScheme.Text);
        painter.drawText(rect(), "Add Chat", QTextOption(Qt::AlignCenter));
    } else {
        painter.fillRect(rect(), this->colorScheme.ChatSeperator);
    }

    QColor accentColor = (QApplication::activeWindow() == this->window()
                              ? this->colorScheme.TabSelectedBackground
                              : this->colorScheme.TabSelectedUnfocusedBackground);

    painter.fillRect(0, 0, width(), 2, accentColor);
}

void NotebookPage::showEvent(QShowEvent *event)
{
    // Whenever this notebook page is shown, give focus to the last focused chat widget
    // If this is the first time this notebook page is shown, it will give focus to the top-left
    // chat widget
    this->requestFocus(this->currentX, this->currentY);
}

static std::pair<int, int> getWidgetPositionInLayout(QLayout *layout, const ChatWidget *chatWidget)
{
    for (int i = 0; i < layout->count(); ++i) {
        printf("xD\n");
    }

    return std::make_pair(-1, -1);
}

std::pair<int, int> NotebookPage::getChatPosition(const ChatWidget *chatWidget)
{
    auto layout = this->ui.hbox.layout();

    if (layout == nullptr) {
        return std::make_pair(-1, -1);
    }

    return getWidgetPositionInLayout(layout, chatWidget);
}

ChatWidget *NotebookPage::createChatWidget()
{
    return new ChatWidget(this->channelManager, this);
}

void NotebookPage::refreshTitle()
{
    if (!this->tab->useDefaultBehaviour) {
        return;
    }

    QString newTitle = "";
    bool first = true;

    for (const auto &chatWidget : this->chatWidgets) {
        auto channelName = QString::fromStdString(chatWidget->channelName.getValue());

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

    this->tab->setTitle(newTitle);
}

void NotebookPage::load(const boost::property_tree::ptree &tree)
{
    try {
        int column = 0;
        for (const auto &v : tree.get_child("columns.")) {
            int row = 0;
            for (const auto &innerV : v.second.get_child("")) {
                auto widget = this->createChatWidget();
                widget->load(innerV.second);
                addToLayout(widget, std::pair<int, int>(column, row));
                ++row;
            }
            ++column;
        }
    } catch (boost::property_tree::ptree_error &) {
        // can't read tabs
    }
}

static void saveFromLayout(QLayout *layout, boost::property_tree::ptree &tree)
{
    for (int i = 0; i < layout->count(); ++i) {
        auto item = layout->itemAt(i);

        auto innerLayout = item->layout();
        if (innerLayout != nullptr) {
            boost::property_tree::ptree innerLayoutTree;

            saveFromLayout(innerLayout, innerLayoutTree);

            if (innerLayoutTree.size() > 0) {
                tree.push_back(std::make_pair("", innerLayoutTree));
            }

            continue;
        }

        auto widget = item->widget();

        if (widget == nullptr) {
            // This layoutitem does not manage a widget for some reason
            continue;
        }

        ChatWidget *chatWidget = qobject_cast<ChatWidget *>(widget);

        if (chatWidget != nullptr) {
            boost::property_tree::ptree chat = chatWidget->save();

            tree.push_back(std::make_pair("", chat));
            continue;
        }
    }
}

boost::property_tree::ptree NotebookPage::save()
{
    boost::property_tree::ptree tree;

    auto layout = this->ui.hbox.layout();

    saveFromLayout(layout, tree);

    /*
    for (const auto &chat : this->chatWidgets) {
        boost::property_tree::ptree child = chat->save();

        // Set child position
        child.put("position", "5,3");

        tree.push_back(std::make_pair("", child));
    }
    */

    return tree;
}

}  // namespace widgets
}  // namespace chatterino
