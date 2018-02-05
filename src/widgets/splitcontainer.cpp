#include "widgets/splitcontainer.hpp"
#include "common.hpp"
#include "singletons/thememanager.hpp"
#include "util/helpers.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/helper/notebooktab.hpp"
#include "widgets/notebook.hpp"
#include "widgets/split.hpp"

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

bool SplitContainer::isDraggingSplit = false;
Split *SplitContainer::draggingSplit = nullptr;
std::pair<int, int> SplitContainer::dropPosition = std::pair<int, int>(-1, -1);

SplitContainer::SplitContainer(Notebook *parent, NotebookTab *_tab, const std::string &_uuid)
    : BaseWidget(parent->themeManager, parent)
    , uuid(_uuid)
    , settingRoot(fS("/containers/{}", this->uuid))
    , tab(_tab)
    , chats(fS("{}/chats", this->settingRoot))
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

    this->loadSplits();

    this->refreshTitle();
}

void SplitContainer::updateFlexValues()
{
    for (int i = 0; i < this->ui.hbox.count(); i++) {
        QVBoxLayout *vbox = (QVBoxLayout *)ui.hbox.itemAt(i)->layout();

        if (vbox->count() != 0) {
            ui.hbox.setStretch(i, (int)(1000 * ((Split *)vbox->itemAt(0))->getFlexSizeX()));
        }
    }
}

int SplitContainer::splitCount() const
{
    return this->splits.size();
}

std::pair<int, int> SplitContainer::removeFromLayout(Split *widget)
{
    // remove reference to chat widget from chatWidgets vector
    auto it = std::find(std::begin(this->splits), std::end(this->splits), widget);
    if (it != std::end(this->splits)) {
        this->splits.erase(it);

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

void SplitContainer::addToLayout(Split *widget, std::pair<int, int> position)
{
    this->splits.push_back(widget);

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

const std::vector<Split *> &SplitContainer::getSplits() const
{
    return this->splits;
}

NotebookTab *SplitContainer::getTab() const
{
    return this->tab;
}

void SplitContainer::addChat(bool openChannelNameDialog, std::string chatUUID)
{
    if (chatUUID.empty()) {
        chatUUID = CreateUUID().toStdString();
    }
    Split *w = this->createChatWidget(chatUUID);

    if (openChannelNameDialog) {
        bool ret = w->showChangeChannelPopup("Open channel name", true);

        if (!ret) {
            delete w;
            return;
        }
    }

    this->addToLayout(w, std::pair<int, int>(-1, -1));
}

void SplitContainer::refreshCurrentFocusCoordinates(bool alsoSetLastRequested)
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
                Split *chatWidget = static_cast<Split *>(w);
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

void SplitContainer::requestFocus(int requestedX, int requestedY)
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
        Split *chatWidget = static_cast<Split *>(w);
        chatWidget->giveFocus(Qt::OtherFocusReason);
    }
}

void SplitContainer::enterEvent(QEvent *)
{
    if (this->ui.hbox.count() == 0) {
        this->setCursor(QCursor(Qt::PointingHandCursor));
    } else {
        this->setCursor(QCursor(Qt::ArrowCursor));
    }
}

void SplitContainer::leaveEvent(QEvent *)
{
}

void SplitContainer::mouseReleaseEvent(QMouseEvent *event)
{
    if (this->ui.hbox.count() == 0 && event->button() == Qt::LeftButton) {
        // "Add Chat" was clicked
        this->addChat(true);

        this->setCursor(QCursor(Qt::ArrowCursor));
    }
}

void SplitContainer::dragEnterEvent(QDragEnterEvent *event)
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

void SplitContainer::dragMoveEvent(QDragMoveEvent *event)
{
    setPreviewRect(event->pos());
}

void SplitContainer::setPreviewRect(QPoint mousePos)
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

void SplitContainer::dragLeaveEvent(QDragLeaveEvent *event)
{
    this->dropPreview.hide();
}

void SplitContainer::dropEvent(QDropEvent *event)
{
    if (isDraggingSplit) {
        event->acceptProposedAction();

        SplitContainer::draggingSplit->setParent(this);

        addToLayout(SplitContainer::draggingSplit, dropPosition);
    }

    this->dropPreview.hide();
}

bool SplitContainer::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);

        this->refreshCurrentFocusCoordinates((focusEvent->reason() == Qt::MouseFocusReason));
    }

    return false;
}

void SplitContainer::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (this->ui.hbox.count() == 0) {
        painter.fillRect(rect(), this->themeManager.splits.background);

        painter.setPen(this->themeManager.splits.header.text);

        QString text = "Click to add a <split>";

        Notebook *notebook = dynamic_cast<Notebook *>(this->parentWidget());

        if (notebook != nullptr) {
            if (notebook->tabCount() > 1) {
                text += "\n\ntip: you can drag a <split> while holding <Alt>";
                text += "\nor add another one by pressing <Ctrl+T>";
            }
        }

        painter.drawText(rect(), text, QTextOption(Qt::AlignCenter));
    } else {
        painter.fillRect(rect(), this->themeManager.splits.messageSeperator);
    }

    QBrush accentColor = (QApplication::activeWindow() == this->window()
                              ? this->themeManager.tabs.selected.backgrounds.regular
                              : this->themeManager.tabs.selected.backgrounds.unfocused);

    painter.fillRect(0, 0, width(), 2, accentColor);
}

void SplitContainer::showEvent(QShowEvent *event)
{
    // Whenever this notebook page is shown, give focus to the last focused chat widget
    // If this is the first time this notebook page is shown, it will give focus to the top-left
    // chat widget
    this->requestFocus(this->currentX, this->currentY);
}

static std::pair<int, int> getWidgetPositionInLayout(QLayout *layout, const Split *chatWidget)
{
    for (int i = 0; i < layout->count(); ++i) {
        printf("xD\n");
    }

    return std::make_pair(-1, -1);
}

std::pair<int, int> SplitContainer::getChatPosition(const Split *chatWidget)
{
    auto layout = this->ui.hbox.layout();

    if (layout == nullptr) {
        return std::make_pair(-1, -1);
    }

    return getWidgetPositionInLayout(layout, chatWidget);
}

Split *SplitContainer::createChatWidget(const std::string &uuid)
{
    auto split = new Split(this, uuid);

    split->getChannelView().highlightedMessageReceived.connect([this] {
        // fourtf: error potentionally here
        this->tab->setHighlightState(HighlightState::Highlighted);  //
    });

    return split;
}

void SplitContainer::refreshTitle()
{
    if (!this->tab->useDefaultBehaviour) {
        return;
    }

    QString newTitle = "";
    bool first = true;

    for (const auto &chatWidget : this->splits) {
        auto channelName = chatWidget->channelName.getValue();
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

void SplitContainer::loadSplits()
{
    const auto hboxes = this->chats.getValue();
    int column = 0;
    for (const std::vector<std::string> &hbox : hboxes) {
        int row = 0;
        for (const std::string &chatUUID : hbox) {
            Split *split = this->createChatWidget(chatUUID);

            this->addToLayout(split, std::pair<int, int>(column, row));

            ++row;
        }
        ++column;
    }
}

template <typename Container>
static void saveFromLayout(QLayout *layout, Container &container)
{
    for (int i = 0; i < layout->count(); ++i) {
        auto item = layout->itemAt(i);

        auto innerLayout = item->layout();
        if (innerLayout != nullptr) {
            std::vector<std::string> vbox;

            for (int j = 0; j < innerLayout->count(); ++j) {
                auto innerItem = innerLayout->itemAt(j);
                auto innerWidget = innerItem->widget();
                if (innerWidget == nullptr) {
                    assert(false);
                    continue;
                }
                Split *innerSplit = qobject_cast<Split *>(innerWidget);
                vbox.push_back(innerSplit->getUUID());
            }

            container.push_back(vbox);

            continue;
        }
    }
}

void SplitContainer::save()
{
    auto layout = this->ui.hbox.layout();

    std::vector<std::vector<std::string>> _chats;

    for (int i = 0; i < layout->count(); ++i) {
        auto item = layout->itemAt(i);

        auto innerLayout = item->layout();
        if (innerLayout != nullptr) {
            std::vector<std::string> vbox;

            for (int j = 0; j < innerLayout->count(); ++j) {
                auto innerItem = innerLayout->itemAt(j);
                auto innerWidget = innerItem->widget();
                if (innerWidget == nullptr) {
                    assert(false);
                    continue;
                }
                Split *innerSplit = qobject_cast<Split *>(innerWidget);
                vbox.push_back(innerSplit->getUUID());
            }

            _chats.push_back(vbox);

            continue;
        }
    }

    this->chats = _chats;
}

}  // namespace widgets
}  // namespace chatterino
