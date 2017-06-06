#include "widgets/notebookpage.h"
#include "colorscheme.h"
#include "widgets/chatwidget.h"
#include "widgets/notebooktab.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QMimeData>
#include <QObject>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/foreach.hpp>

namespace chatterino {
namespace widgets {

bool NotebookPage::isDraggingSplit = false;
ChatWidget *NotebookPage::draggingSplit = NULL;
std::pair<int, int> NotebookPage::dropPosition = std::pair<int, int>(-1, -1);

NotebookPage::NotebookPage(QWidget *parent, NotebookTab *tab)
    : QWidget(parent)
    , _tab(tab)
    , _parentbox(this)
    , _chatWidgets()
    , _preview(this)
{
    tab->page = this;

    setHidden(true);
    setAcceptDrops(true);

    _parentbox.addSpacing(2);
    _parentbox.addLayout(&_hbox);
    _parentbox.setMargin(0);

    _hbox.setSpacing(1);
    _hbox.setMargin(0);
}

const std::vector<ChatWidget *> &NotebookPage::getChatWidgets() const
{
    return _chatWidgets;
}

NotebookTab *NotebookPage::getTab() const
{
    return _tab;
}

void
NotebookPage::addChat(bool openChannelNameDialog)
{
    ChatWidget *w = new ChatWidget();

    if (openChannelNameDialog) {
        w->showChangeChannelPopup();
    }

    addToLayout(w, std::pair<int, int>(-1, -1));
}

std::pair<int, int> NotebookPage::removeFromLayout(ChatWidget *widget)
{
    // remove from chatWidgets vector
    for (auto it = _chatWidgets.begin(); it != _chatWidgets.end(); ++it) {
        if (*it == widget) {
            _chatWidgets.erase(it);

            break;
        }
    }

    // remove from box and return location
    for (int i = 0; i < _hbox.count(); ++i) {
        auto vbox = static_cast<QVBoxLayout *>(_hbox.itemAt(i));

        for (int j = 0; j < vbox->count(); ++j) {
            if (vbox->itemAt(j)->widget() != widget) {
                continue;
            }

            widget->setParent(NULL);

            bool isLastItem = vbox->count() == 0;

            if (isLastItem) {
                _hbox.removeItem(vbox);

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
    _chatWidgets.push_back(widget);

    // add vbox at the end
    if (position.first < 0 || position.first >= _hbox.count()) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        _hbox.addLayout(vbox, 1);
        return;
    }

    // insert vbox
    if (position.second == -1) {
        auto vbox = new QVBoxLayout();
        vbox->addWidget(widget);

        _hbox.insertLayout(position.first, vbox, 1);
        return;
    }

    // add to existing vbox
    auto vbox = static_cast<QVBoxLayout *>(_hbox.itemAt(position.first));

    vbox->insertWidget(std::max(0, std::min(vbox->count(), position.second)), widget);
}

void NotebookPage::enterEvent(QEvent *)
{
    if (_hbox.count() == 0) {
        setCursor(QCursor(Qt::PointingHandCursor));
    } else {
        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void NotebookPage::leaveEvent(QEvent *)
{
}

void NotebookPage::mouseReleaseEvent(QMouseEvent *event)
{
    if (_hbox.count() == 0 && event->button() == Qt::LeftButton) {
        // "Add Chat" was clicked
        addToLayout(new ChatWidget(), std::pair<int, int>(-1, -1));

        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void NotebookPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
        return;

    if (isDraggingSplit) {
        return;
    }

    _dropRegions.clear();

    if (_hbox.count() == 0) {
        _dropRegions.push_back(DropRegion(rect(), std::pair<int, int>(-1, -1)));
    } else {
        for (int i = 0; i < _hbox.count() + 1; ++i) {
            _dropRegions.push_back(DropRegion(QRect(((i * 4 - 1) * width() / _hbox.count()) / 4, 0,
                                                    width() / _hbox.count() / 2 + 1, height() + 1),
                                              std::pair<int, int>(i, -1)));
        }

        for (int i = 0; i < _hbox.count(); ++i) {
            auto vbox = static_cast<QVBoxLayout *>(_hbox.itemAt(i));

            for (int j = 0; j < vbox->count() + 1; ++j) {
                _dropRegions.push_back(DropRegion(
                    QRect(i * width() / _hbox.count(), ((j * 2 - 1) * height() / vbox->count()) / 2,
                          width() / _hbox.count() + 1, height() / vbox->count() + 1),

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
    for (DropRegion region : _dropRegions) {
        if (region.rect.contains(mousePos)) {
            _preview.setBounds(region.rect);

            if (!_preview.isVisible()) {
                _preview.show();
                _preview.raise();
            }

            dropPosition = region.position;

            return;
        }
    }

    _preview.hide();
}

void NotebookPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    _preview.hide();
}

void NotebookPage::dropEvent(QDropEvent *event)
{
    if (isDraggingSplit) {
        event->acceptProposedAction();

        NotebookPage::draggingSplit->setParent(this);

        addToLayout(NotebookPage::draggingSplit, dropPosition);
    }

    _preview.hide();
}

void NotebookPage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (_hbox.count() == 0) {
        painter.fillRect(rect(), ColorScheme::getInstance().ChatBackground);

        painter.fillRect(0, 0, width(), 2, ColorScheme::getInstance().TabSelectedBackground);

        painter.setPen(ColorScheme::getInstance().Text);
        painter.drawText(rect(), "Add Chat", QTextOption(Qt::AlignCenter));
    } else {
        painter.fillRect(rect(), ColorScheme::getInstance().TabSelectedBackground);

        painter.fillRect(0, 0, width(), 2, ColorScheme::getInstance().TabSelectedBackground);
    }
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
    auto layout = _hbox.layout();

    if (layout == nullptr) {
        return std::make_pair(-1, -1);
    }

    return getWidgetPositionInLayout(layout, chatWidget);
}

void NotebookPage::load(const boost::property_tree::ptree &tree)
{
    try {
        int column = 0;
        for (const auto &v : tree.get_child("columns.")) {
            int row = 0;
            for (const auto &innerV : v.second.get_child("")) {
                auto widget = new ChatWidget();
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

    auto layout = _hbox.layout();

    saveFromLayout(layout, tree);

    /*
    for (const auto &chat : chatWidgets) {
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
