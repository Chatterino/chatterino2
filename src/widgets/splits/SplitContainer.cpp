#include "widgets/splits/SplitContainer.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/QLogging.hpp"
#include "common/WindowDescriptors.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/QMagicEnum.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/ClosedSplits.hpp"
#include "widgets/splits/DraggedSplit.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QMimeData>
#include <QPainter>

#include <algorithm>

namespace chatterino {

SplitContainer::SplitContainer(Notebook *parent)
    : BaseWidget(parent)
    , overlay_(this)
    , mouseOverPoint_(-10000, -10000)
    , tab_(nullptr)
{
    this->refreshTabTitle();

    this->signalHolder_.managedConnect(
        Split::modifierStatusChanged, [this](auto modifiers) {
            this->layout();

            if (modifiers == SHOW_RESIZE_HANDLES_MODIFIERS)
            {
                for (auto &handle : this->resizeHandles_)
                {
                    handle->show();
                    handle->raise();
                }
            }
            else
            {
                for (auto &handle : this->resizeHandles_)
                {
                    handle->hide();
                }
            }

            if (modifiers == SHOW_SPLIT_OVERLAY_MODIFIERS)
            {
                this->setCursor(Qt::PointingHandCursor);
            }
            else
            {
                this->unsetCursor();
            }
        });

    this->setCursor(Qt::PointingHandCursor);
    this->setAcceptDrops(true);

    this->signalHolder_.managedConnect(this->overlay_.dragEnded, [this]() {
        this->isDragging_ = false;
        this->layout();
    });

    this->overlay_.hide();

    this->setMouseTracking(true);
    this->setAcceptDrops(true);
}

NotebookTab *SplitContainer::getTab() const
{
    return this->tab_;
}

void SplitContainer::setTab(NotebookTab *tab)
{
    this->tab_ = tab;

    this->tab_->page = this;

    this->refreshTab();
}

void SplitContainer::hideResizeHandles()
{
    this->overlay_.hide();

    for (auto &handle : this->resizeHandles_)
    {
        handle->hide();
    }
}

void SplitContainer::resetMouseStatus()
{
    this->mouseOverPoint_ = QPoint(-10000, -10000);
    this->update();
}

Split *SplitContainer::appendNewSplit(bool openChannelNameDialog)
{
    assertInGuiThread();

    auto *split = new Split(this);
    this->insertSplit(split);

    if (openChannelNameDialog)
    {
        split->showChangeChannelPopup("Open channel", true, [=, this](bool ok) {
            if (!ok)
            {
                this->deleteSplit(split);
            }
        });
    }

    return split;
}

void SplitContainer::insertSplit(Split *split, InsertOptions &&options)
{
    // Queue up save because: Split added
    getApp()->getWindows()->queueSave();

    assertInGuiThread();

    if (options.position)
    {
        // options.position must not be set together with any other options
        assert(!options.relativeSplit);
        assert(!options.relativeNode);
        assert(!options.direction.has_value());

        options.relativeNode = options.position->relativeNode_;
        options.direction = options.position->direction_;
    }

    if (options.relativeSplit)
    {
        // options.relativeNode must not be set together with relativeSplit
        assert(!options.relativeNode);

        Node *node =
            this->baseNode_.findNodeContainingSplit(options.relativeSplit);
        assert(node != nullptr);

        options.relativeNode = node;
    }

    auto *relativeTo = options.relativeNode;
    const auto direction = options.direction.value_or(SplitDirection::Right);

    if (relativeTo == nullptr)
    {
        if (this->baseNode_.type_ == Node::Type::EmptyRoot)
        {
            this->baseNode_.setSplit(split);
        }
        else if (this->baseNode_.type_ == Node::Type::Split)
        {
            this->baseNode_.nestSplitIntoCollection(split, direction);
        }
        else
        {
            this->baseNode_.insertSplitRelative(split, direction);
        }
    }
    else
    {
        assert(this->baseNode_.isOrContainsNode(relativeTo));

        relativeTo->insertSplitRelative(split, direction);
    }

    this->addSplit(split);
}

Split *SplitContainer::getSelectedSplit() const
{
    // safety check
    if (std::find(this->splits_.begin(), this->splits_.end(),
                  this->selected_) == this->splits_.end())
    {
        return nullptr;
    }

    return this->selected_;
}

void SplitContainer::addSplit(Split *split)
{
    assertInGuiThread();

    split->setParent(this);
    split->show();
    split->setFocus(Qt::FocusReason::MouseFocusReason);
    this->unsetCursor();
    this->splits_.push_back(split);

    this->refreshTab();

    auto &&conns = this->connectionsPerSplit_[split];

    conns.managedConnect(split->getChannelView().tabHighlightRequested,
                         [this, split](HighlightState state) {
                             if (this->tab_ != nullptr)
                             {
                                 this->tab_->updateHighlightState(
                                     state, split->getChannelView());
                             }
                         });

    conns.managedConnect(split->channelChanged, [this, split] {
        if (this->tab_ != nullptr)
        {
            this->tab_->newHighlightSourceAdded(split->getChannelView());
        }
    });

    conns.managedConnect(split->getChannelView().liveStatusChanged, [this]() {
        this->refreshTabLiveStatus();
    });

    conns.managedConnect(split->focused, [this, split] {
        this->setSelected(split);
    });

    conns.managedConnect(split->openSplitRequested, [this](auto channel) {
        this->appendNewSplit(false)->setChannel(channel);
    });

    conns.managedConnect(
        split->actionRequested, [this, split](Split::Action action) {
            switch (action)
            {
                case Split::Action::RefreshTab:
                    this->refreshTab();
                    break;

                case Split::Action::ResetMouseStatus:
                    this->resetMouseStatus();
                    break;

                case Split::Action::AppendNewSplit:
                    this->appendNewSplit(true);
                    break;

                case Split::Action::Delete: {
                    this->deleteSplit(split);
                    auto *tab = this->getTab();
                    QObject::connect(tab, &QWidget::destroyed, [tab]() mutable {
                        ClosedSplits::invalidateTab(tab);
                    });
                    ClosedSplits::push({split->getChannel()->getName(),
                                        split->getFilters(), tab});
                }
                break;

                case Split::Action::SelectSplitLeft:
                    this->selectNextSplit(SplitDirection::Left);
                    break;
                case Split::Action::SelectSplitRight:
                    this->selectNextSplit(SplitDirection::Right);
                    break;
                case Split::Action::SelectSplitAbove:
                    this->selectNextSplit(SplitDirection::Above);
                    break;
                case Split::Action::SelectSplitBelow:
                    this->selectNextSplit(SplitDirection::Below);
                    break;
            }
        });

    conns.managedConnect(
        split->insertSplitRequested, [this](SplitDirection dir, Split *parent) {
            this->insertSplit(new Split(this), {
                                                   .relativeSplit = parent,
                                                   .direction = dir,
                                               });
        });

    this->layout();
}

void SplitContainer::setSelected(Split *split)
{
    // safety
    if (std::find(this->splits_.begin(), this->splits_.end(), split) ==
        this->splits_.end())
    {
        return;
    }

    this->selected_ = split;

    if (Node *node = this->baseNode_.findNodeContainingSplit(split))
    {
        this->focusSplitRecursive(node);
        this->setPreferedTargetRecursive(node);
    }
}

void SplitContainer::setPreferedTargetRecursive(Node *node)
{
    if (node->parent_ != nullptr)
    {
        node->parent_->preferedFocusTarget_ = node;

        this->setPreferedTargetRecursive(node->parent_);
    }
}

SplitContainer::Position SplitContainer::releaseSplit(Split *split)
{
    assertInGuiThread();

    Node *node = this->baseNode_.findNodeContainingSplit(split);
    assert(node != nullptr);

    this->splits_.erase(
        std::find(this->splits_.begin(), this->splits_.end(), split));
    split->setParent(nullptr);
    Position position = node->releaseSplit();
    this->layout();
    if (splits_.empty())
    {
        this->setSelected(nullptr);
        this->setCursor(Qt::PointingHandCursor);
    }
    else
    {
        this->splits_.front()->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    this->refreshTab();

    this->connectionsPerSplit_.erase(this->connectionsPerSplit_.find(split));

    return position;
}

SplitContainer::Position SplitContainer::deleteSplit(Split *split)
{
    // Queue up save because: Split removed
    getApp()->getWindows()->queueSave();

    assertInGuiThread();
    assert(split != nullptr);

    split->deleteLater();
    return releaseSplit(split);
}

void SplitContainer::selectNextSplit(SplitDirection direction)
{
    assertInGuiThread();

    if (Node *node = this->baseNode_.findNodeContainingSplit(this->selected_))
    {
        this->selectSplitRecursive(node, direction);
    }
}

void SplitContainer::selectSplitRecursive(Node *node, SplitDirection direction)
{
    if (node->parent_ != nullptr)
    {
        if (node->parent_->type_ == Node::toContainerType(direction))
        {
            auto &siblings = node->parent_->children_;

            auto it = std::find_if(siblings.begin(), siblings.end(),
                                   [node](const auto &other) {
                                       return other.get() == node;
                                   });
            assert(it != siblings.end());

            if (direction == SplitDirection::Left ||
                direction == SplitDirection::Above)
            {
                if (it == siblings.begin())
                {
                    this->selectSplitRecursive(node->parent_, direction);
                }
                else
                {
                    this->focusSplitRecursive(
                        siblings[it - siblings.begin() - 1].get());
                }
            }
            else
            {
                if (it->get() == siblings.back().get())
                {
                    this->selectSplitRecursive(node->parent_, direction);
                }
                else
                {
                    this->focusSplitRecursive(
                        siblings[it - siblings.begin() + 1].get());
                }
            }
        }
        else
        {
            this->selectSplitRecursive(node->parent_, direction);
        }
    }
}

void SplitContainer::focusSplitRecursive(Node *node)
{
    switch (node->type_)
    {
        case Node::Type::Split: {
            node->split_->setFocus(Qt::FocusReason::OtherFocusReason);
        }
        break;

        case Node::Type::HorizontalContainer:
        case Node::Type::VerticalContainer: {
            auto &children = node->children_;

            auto it = std::find_if(
                children.begin(), children.end(), [node](const auto &other) {
                    return node->preferedFocusTarget_ == other.get();
                });

            if (it != children.end())
            {
                this->focusSplitRecursive(it->get());
            }
            else
            {
                this->focusSplitRecursive(node->children_.front().get());
            }
        }
        break;

        default:;
    }
}

Split *SplitContainer::getTopRightSplit(Node &node)
{
    switch (node.getType())
    {
        case Node::Type::Split:
            return node.getSplit();
        case Node::Type::VerticalContainer:
            if (!node.getChildren().empty())
            {
                return getTopRightSplit(*node.getChildren().front());
            }
            break;
        case Node::Type::HorizontalContainer:
            if (!node.getChildren().empty())
            {
                return getTopRightSplit(*node.getChildren().back());
            }
            break;
        default:;
    }
    return nullptr;
}

void SplitContainer::layout()
{
    if (this->disableLayouting_)
    {
        return;
    }

    // update top right split
    auto *topRight = this->getTopRightSplit(this->baseNode_);
    if (this->topRight_)
    {
        this->topRight_->setIsTopRightSplit(false);
    }
    this->topRight_ = topRight;
    if (topRight)
    {
        this->topRight_->setIsTopRightSplit(true);
    }

    // layout
    this->baseNode_.geometry_ = this->rect().adjusted(-1, -1, 0, 0);

    std::vector<DropRect> dropRects;
    std::vector<ResizeRect> resizeRects;

    const bool addSpacing =
        Split::modifierStatus == SHOW_ADD_SPLIT_REGIONS || this->isDragging_;
    this->baseNode_.layout(addSpacing, this->scale(), dropRects, resizeRects);

    this->dropRects_ = dropRects;

    for (Split *split : this->splits_)
    {
        const QRect &g = split->geometry();

        Node *node = this->baseNode_.findNodeContainingSplit(split);

        // left
        dropRects.emplace_back(
            QRect(g.left(), g.top(), g.width() / 3, g.height()),
            Position(node, SplitDirection::Left));
        // right
        dropRects.emplace_back(QRect(g.right() - g.width() / 3, g.top(),
                                     g.width() / 3, g.height()),
                               Position(node, SplitDirection::Right));

        // top
        dropRects.emplace_back(
            QRect(g.left(), g.top(), g.width(), g.height() / 2),
            Position(node, SplitDirection::Above));
        // bottom
        dropRects.emplace_back(QRect(g.left(), g.bottom() - g.height() / 2,
                                     g.width(), g.height() / 2),
                               Position(node, SplitDirection::Below));
    }

    if (this->splits_.empty())
    {
        QRect g = this->rect();
        dropRects.emplace_back(
            QRect(g.left(), g.top(), g.width() - 1, g.height() - 1),
            Position(nullptr, SplitDirection::Below));
    }

    this->overlay_.setRects(std::move(dropRects));

    // handle resizeHandles
    if (this->resizeHandles_.size() < resizeRects.size())
    {
        while (this->resizeHandles_.size() < resizeRects.size())
        {
            this->resizeHandles_.push_back(
                std::make_unique<ResizeHandle>(this));
        }
    }
    else if (this->resizeHandles_.size() > resizeRects.size())
    {
        this->resizeHandles_.resize(resizeRects.size());
    }

    {
        size_t i = 0;
        for (ResizeRect &resizeRect : resizeRects)
        {
            ResizeHandle *handle = this->resizeHandles_[i].get();
            handle->setGeometry(resizeRect.rect);
            handle->setVertical(resizeRect.vertical);
            handle->node = resizeRect.node;

            if (Split::modifierStatus == SHOW_RESIZE_HANDLES_MODIFIERS)
            {
                handle->show();
                handle->raise();
            }

            i++;
        }
    }

    // redraw
    this->update();
}

void SplitContainer::resizeEvent(QResizeEvent *event)
{
    BaseWidget::resizeEvent(event);

    this->layout();
}

void SplitContainer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (this->splits_.empty())
        {
            // "Add Chat" was clicked
            this->appendNewSplit(true);
            this->mouseOverPoint_ = QPoint(-10000, -10000);

            //            this->setCursor(QCursor(Qt::ArrowCursor));
        }
        else
        {
            auto it =
                std::find_if(this->dropRects_.begin(), this->dropRects_.end(),
                             [event](DropRect &rect) {
                                 return rect.rect.contains(event->pos());
                             });
            if (it != this->dropRects_.end())
            {
                this->insertSplit(new Split(this), {.position = it->position});
            }
        }
    }
}

void SplitContainer::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    if (this->splits_.empty())
    {
        painter.fillRect(rect(), this->theme->splits.background);

        painter.setPen(this->theme->splits.header.text);

        const auto font =
            getApp()->getFonts()->getFont(FontStyle::ChatMedium, this->scale());
        painter.setFont(font);

        QString text = "Click to add a split";

        auto *notebook = dynamic_cast<Notebook *>(this->parentWidget());

        if (notebook != nullptr)
        {
            if (notebook->getPageCount() > 1)
            {
                text += "\n\nAfter adding hold <Ctrl+Alt> to move or split it.";
            }
        }

        painter.drawText(rect(), text, QTextOption(Qt::AlignCenter));
    }
    else
    {
        if (getApp()->getThemes()->isLightTheme())
        {
            painter.fillRect(rect(), QColor("#999"));
        }
        else
        {
            painter.fillRect(rect(), QColor("#555"));
        }
    }

    for (DropRect &dropRect : this->dropRects_)
    {
        QColor border = getApp()->getThemes()->splits.dropTargetRectBorder;
        QColor background = getApp()->getThemes()->splits.dropTargetRect;

        if (!dropRect.rect.contains(this->mouseOverPoint_))
        {
            //            border.setAlphaF(0.1);
            //            background.setAlphaF(0.1);
        }
        else
        {
            //            background.setAlphaF(0.1);
            border.setAlpha(255);
        }

        painter.setPen(border);
        painter.setBrush(background);

        auto rect = dropRect.rect.marginsRemoved(QMargins(2, 2, 2, 2));

        painter.drawRect(rect);

        int s =
            std::min<int>(dropRect.rect.width(), dropRect.rect.height()) - 12;

        if (this->theme->isLightTheme())
        {
            painter.setPen(QColor(0, 0, 0));
        }
        else
        {
            painter.setPen(QColor(255, 255, 255));
        }
        painter.drawLine(rect.left() + rect.width() / 2 - (s / 2),
                         rect.top() + rect.height() / 2,
                         rect.left() + rect.width() / 2 + (s / 2),
                         rect.top() + rect.height() / 2);
        painter.drawLine(rect.left() + rect.width() / 2,
                         rect.top() + rect.height() / 2 - (s / 2),
                         rect.left() + rect.width() / 2,
                         rect.top() + rect.height() / 2 + (s / 2));
    }

    auto accentColor = (QApplication::activeWindow() == this->window()
                            ? this->theme->tabs.selected.backgrounds.regular
                            : this->theme->tabs.selected.backgrounds.unfocused);

    painter.fillRect(0, 0, width(), 1, accentColor);
}

void SplitContainer::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat("chatterino/split"))
    {
        return;
    }

    if (!isDraggingSplit())
    {
        return;
    }

    this->isDragging_ = true;
    this->layout();

    this->overlay_.setGeometry(this->rect());
    this->overlay_.show();
    this->overlay_.raise();
}

void SplitContainer::mouseMoveEvent(QMouseEvent *event)
{
    if (Split::modifierStatus == SHOW_SPLIT_OVERLAY_MODIFIERS)
    {
        this->setCursor(Qt::PointingHandCursor);
    }

    this->mouseOverPoint_ = event->pos();
    this->update();
}

void SplitContainer::leaveEvent(QEvent * /*event*/)
{
    this->mouseOverPoint_ = QPoint(-10000, -10000);
    this->update();
}

void SplitContainer::focusInEvent(QFocusEvent * /*event*/)
{
    if (this->baseNode_.findNodeContainingSplit(this->selected_) != nullptr)
    {
        this->selected_->setFocus();
        return;
    }

    if (!this->splits_.empty())
    {
        this->splits_.front()->setFocus();
    }
}

void SplitContainer::refreshTab()
{
    this->refreshTabTitle();
    this->refreshTabLiveStatus();
}

std::vector<Split *> SplitContainer::getSplits() const
{
    return this->splits_;
}

SplitContainer::Node *SplitContainer::getBaseNode()
{
    return &this->baseNode_;
}

NodeDescriptor SplitContainer::buildDescriptor() const
{
    return this->buildDescriptorRecursively(&this->baseNode_);
}

void SplitContainer::applyFromDescriptor(const NodeDescriptor &rootNode)
{
    assert(this->baseNode_.type_ == Node::Type::EmptyRoot);

    this->disableLayouting_ = true;
    this->applyFromDescriptorRecursively(rootNode, &this->baseNode_);
    this->disableLayouting_ = false;
    this->layout();
}

void SplitContainer::popup()
{
    Window &window = getApp()->getWindows()->createWindow(WindowType::Popup);
    auto *popupContainer = window.getNotebook().getOrAddSelectedPage();

    QJsonObject encodedTab;
    WindowManager::encodeTab(this, true, encodedTab);
    TabDescriptor tab = TabDescriptor::loadFromJSON(encodedTab);

    // custom title
    if (!tab.customTitle_.isEmpty())
    {
        popupContainer->getTab()->setCustomTitle(tab.customTitle_);
    }

    // highlighting on new messages
    popupContainer->getTab()->setHighlightsEnabled(tab.highlightsEnabled_);

    // splits
    if (tab.rootNode_)
    {
        popupContainer->applyFromDescriptor(*tab.rootNode_);
    }

    window.show();
}

QString channelTypeToString(Channel::Type value) noexcept
{
    using Type = chatterino::Channel::Type;
    switch (value)
    {
        default:
            assert(false && "value cannot be serialized");
            return "never";

        case Type::Twitch:
            return "twitch";
        case Type::TwitchWhispers:
            return "whispers";
        case Type::TwitchWatching:
            return "watching";
        case Type::TwitchMentions:
            return "mentions";
        case Type::TwitchLive:
            return "live";
        case Type::TwitchAutomod:
            return "automod";
        case Type::Misc:
            return "misc";
    }
}

NodeDescriptor SplitContainer::buildDescriptorRecursively(
    const Node *currentNode) const
{
    if (currentNode->children_.empty())
    {
        const auto channelType =
            currentNode->split_->getIndirectChannel().getType();

        SplitNodeDescriptor result;
        result.type_ = channelTypeToString(channelType);
        result.channelName_ = currentNode->split_->getChannel()->getName();
        result.filters_ = currentNode->split_->getFilters();
        return result;
    }

    ContainerNodeDescriptor descriptor;
    for (const auto &child : currentNode->children_)
    {
        descriptor.vertical_ =
            currentNode->type_ == Node::Type::VerticalContainer;
        descriptor.items_.push_back(
            this->buildDescriptorRecursively(child.get()));
    }

    return descriptor;
}

void SplitContainer::applyFromDescriptorRecursively(
    const NodeDescriptor &rootNode, Node *baseNode)
{
    if (std::holds_alternative<SplitNodeDescriptor>(rootNode))
    {
        // This is a leaf, no further recursion happens from here

        const auto *n = std::get_if<SplitNodeDescriptor>(&rootNode);
        if (!n)
        {
            return;
        }
        const auto &splitNode = *n;

        auto *split = new Split(this);
        split->setChannel(WindowManager::decodeChannel(splitNode));
        split->setModerationMode(splitNode.moderationMode_);
        split->setFilters(splitNode.filters_);

        this->insertSplit(split);

        return;
    }

    if (std::holds_alternative<ContainerNodeDescriptor>(rootNode))
    {
        // This is a branch, it will contain one or more splits/containers
        const auto *n = std::get_if<ContainerNodeDescriptor>(&rootNode);
        if (!n)
        {
            return;
        }
        const auto &containerNode = *n;

        bool vertical = containerNode.vertical_;

        baseNode->type_ = vertical ? Node::Type::VerticalContainer
                                   : Node::Type::HorizontalContainer;

        for (const auto &item : containerNode.items_)
        {
            if (std::holds_alternative<SplitNodeDescriptor>(item))
            {
                const auto *inner = std::get_if<SplitNodeDescriptor>(&item);
                if (!inner)
                {
                    return;
                }
                const auto &splitNode = *inner;
                auto *split = new Split(this);
                split->setFilters(splitNode.filters_);
                split->setChannel(WindowManager::decodeChannel(splitNode));
                split->setModerationMode(splitNode.moderationMode_);

                auto *node = new Node();
                node->parent_ = baseNode;
                node->split_ = split;
                node->type_ = Node::Type::Split;

                node->flexH_ = splitNode.flexH_;
                node->flexV_ = splitNode.flexV_;
                baseNode->children_.emplace_back(node);

                this->addSplit(split);
            }
            else
            {
                auto *node = new Node();
                node->parent_ = baseNode;

                if (const auto *inner =
                        std::get_if<ContainerNodeDescriptor>(&item))
                {
                    node->flexH_ = inner->flexH_;
                    node->flexV_ = inner->flexV_;
                }

                baseNode->children_.emplace_back(node);
                this->applyFromDescriptorRecursively(item, node);
            }
        }
    }
}

void SplitContainer::refreshTabTitle()
{
    if (this->tab_ == nullptr)
    {
        return;
    }

    QString newTitle = "";
    bool first = true;

    for (const auto &chatWidget : this->splits_)
    {
        auto channelName = chatWidget->getChannel()->getLocalizedName();
        if (channelName.isEmpty())
        {
            continue;
        }

        if (!first)
        {
            newTitle += ", ";
        }
        newTitle += channelName;

        first = false;
    }

    if (newTitle.isEmpty())
    {
        newTitle = "empty";
    }

    this->tab_->setDefaultTitle(newTitle);
}

void SplitContainer::refreshTabLiveStatus()
{
    if (this->tab_ == nullptr)
    {
        return;
    }

    bool liveStatus = false;
    bool rerunStatus = false;
    for (const auto &s : this->splits_)
    {
        auto c = s->getChannel();
        if (c->isRerun())
        {
            rerunStatus = true;
            continue;  // reruns are also marked as live, SKIP
        }
        if (c->isLive())
        {
            liveStatus = true;
            break;
        }
    }

    if (this->tab_->setLive(liveStatus) || this->tab_->setRerun(rerunStatus))
    {
        auto *notebook = dynamic_cast<Notebook *>(this->parentWidget());
        if (notebook)
        {
            notebook->refresh();
        }
    }
}

//
// Node
//

SplitContainer::Node::Type SplitContainer::Node::getType() const
{
    return this->type_;
}
Split *SplitContainer::Node::getSplit() const
{
    return this->split_;
}

SplitContainer::Node *SplitContainer::Node::getParent() const
{
    return this->parent_;
}

qreal SplitContainer::Node::getHorizontalFlex() const
{
    return this->flexH_;
}

qreal SplitContainer::Node::getVerticalFlex() const
{
    return this->flexV_;
}

const std::vector<std::unique_ptr<SplitContainer::Node>> &
    SplitContainer::Node::getChildren()
{
    return this->children_;
}

SplitContainer::Node::Node()
    : type_(SplitContainer::Node::Type::EmptyRoot)
    , split_(nullptr)
    , parent_(nullptr)
{
}

SplitContainer::Node::Node(Split *_split, Node *_parent)
    : type_(Type::Split)
    , split_(_split)
    , parent_(_parent)
{
}

bool SplitContainer::Node::isOrContainsNode(SplitContainer::Node *_node)
{
    if (this == _node)
    {
        return true;
    }

    return std::any_of(this->children_.begin(), this->children_.end(),
                       [_node](std::unique_ptr<Node> &n) {
                           return n->isOrContainsNode(_node);
                       });
}

SplitContainer::Node *SplitContainer::Node::findNodeContainingSplit(
    Split *_split)
{
    if (this->type_ == Type::Split && this->split_ == _split)
    {
        return this;
    }

    for (std::unique_ptr<Node> &node : this->children_)
    {
        Node *a = node->findNodeContainingSplit(_split);

        if (a != nullptr)
        {
            return a;
        }
    }
    return nullptr;
}

void SplitContainer::Node::insertSplitRelative(Split *_split,
                                               SplitDirection _direction)
{
    if (this->parent_ == nullptr)
    {
        switch (this->type_)
        {
            case Node::Type::EmptyRoot: {
                this->setSplit(_split);
            }
            break;
            case Node::Type::Split: {
                this->nestSplitIntoCollection(_split, _direction);
            }
            break;
            case Node::Type::HorizontalContainer: {
                this->nestSplitIntoCollection(_split, _direction);
            }
            break;
            case Node::Type::VerticalContainer: {
                this->nestSplitIntoCollection(_split, _direction);
            }
            break;
        }
        return;
    }

    // parent != nullptr
    if (parent_->type_ == toContainerType(_direction))
    {
        // hell yeah we'll just insert it next to outselves
        this->insertNextToThis(_split, _direction);
    }
    else
    {
        this->nestSplitIntoCollection(_split, _direction);
    }
}

void SplitContainer::Node::nestSplitIntoCollection(Split *_split,
                                                   SplitDirection _direction)
{
    if (toContainerType(_direction) == this->type_)
    {
        this->children_.emplace_back(new Node(_split, this));
    }
    else
    {
        // we'll need to nest outselves
        // move all our data into a new node
        Node *clone = new Node();
        clone->type_ = this->type_;
        clone->children_ = std::move(this->children_);
        for (std::unique_ptr<Node> &node : clone->children_)
        {
            node->parent_ = clone;
        }
        clone->split_ = this->split_;
        clone->parent_ = this;

        // add the node to our children and change our type
        this->children_.push_back(std::unique_ptr<Node>(clone));
        this->type_ = toContainerType(_direction);
        this->split_ = nullptr;

        clone->insertNextToThis(_split, _direction);
    }
}

void SplitContainer::Node::insertNextToThis(Split *_split,
                                            SplitDirection _direction)
{
    auto &siblings = this->parent_->children_;

    qreal width = this->parent_->geometry_.width() /
                  std::max<qreal>(0.0001, siblings.size());
    qreal height = this->parent_->geometry_.height() /
                   std::max<qreal>(0.0001, siblings.size());

    if (siblings.size() == 1)
    {
        this->geometry_ = QRect(0, 0, int(width), int(height));
    }

    auto it =
        std::find_if(siblings.begin(), siblings.end(), [this](auto &node) {
            return this == node.get();
        });

    assert(it != siblings.end());
    if (_direction == SplitDirection::Right ||
        _direction == SplitDirection::Below)
    {
        it++;
    }

    Node *node = new Node(_split, this->parent_);
    node->geometry_ = QRectF(0, 0, width, height);
    siblings.insert(it, std::unique_ptr<Node>(node));
}

void SplitContainer::Node::setSplit(Split *_split)
{
    assert(this->split_ == nullptr);
    assert(this->children_.empty());

    this->split_ = _split;
    this->type_ = Type::Split;
}

SplitContainer::Position SplitContainer::Node::releaseSplit()
{
    assert(this->type_ == Type::Split);

    if (parent_ == nullptr)
    {
        this->type_ = Type::EmptyRoot;
        this->split_ = nullptr;

        Position pos;
        pos.relativeNode_ = nullptr;
        pos.direction_ = SplitDirection::Right;
        return pos;
    }

    auto &siblings = this->parent_->children_;

    auto it = std::find_if(begin(siblings), end(siblings), [this](auto &node) {
        return this == node.get();
    });
    assert(it != siblings.end());

    Position position;
    if (siblings.size() == 2)
    {
        // delete this and move split to parent
        position.relativeNode_ = this->parent_;
        if (this->parent_->type_ == Type::VerticalContainer)
        {
            position.direction_ = siblings.begin() == it
                                      ? SplitDirection::Above
                                      : SplitDirection::Below;
        }
        else
        {
            position.direction_ = siblings.begin() == it
                                      ? SplitDirection::Left
                                      : SplitDirection::Right;
        }

        auto *parent = this->parent_;
        siblings.erase(it);
        std::unique_ptr<Node> &sibling = siblings.front();
        parent->type_ = sibling->type_;
        parent->split_ = sibling->split_;
        std::vector<std::unique_ptr<Node>> nodes =
            std::move(sibling->children_);
        for (auto &node : nodes)
        {
            node->parent_ = parent;
        }
        parent->children_ = std::move(nodes);
    }
    else
    {
        if (this == siblings.back().get())
        {
            position.direction_ =
                this->parent_->type_ == Type::VerticalContainer
                    ? SplitDirection::Below
                    : SplitDirection::Right;
            siblings.erase(it);
            position.relativeNode_ = siblings.back().get();
        }
        else
        {
            position.relativeNode_ = (it + 1)->get();
            position.direction_ =
                this->parent_->type_ == Type::VerticalContainer
                    ? SplitDirection::Above
                    : SplitDirection::Left;
            siblings.erase(it);
        }
    }

    return position;
}

qreal SplitContainer::Node::getFlex(bool isVertical)
{
    return isVertical ? this->flexV_ : this->flexH_;
}

qreal SplitContainer::Node::getSize(bool isVertical)
{
    return isVertical ? this->geometry_.height() : this->geometry_.width();
}

qreal SplitContainer::Node::getChildrensTotalFlex(bool isVertical)
{
    return std::accumulate(this->children_.begin(), this->children_.end(),
                           qreal(0),
                           [=](qreal val, std::unique_ptr<Node> &node) {
                               return val + node->getFlex(isVertical);
                           });
}

void SplitContainer::Node::layout(bool addSpacing, float _scale,
                                  std::vector<DropRect> &dropRects,
                                  std::vector<ResizeRect> &resizeRects)
{
    for (std::unique_ptr<Node> &node : this->children_)
    {
        node->clamp();
    }

    switch (this->type_)
    {
        case Node::Type::Split: {
            QRect rect = this->geometry_.toRect();
            this->split_->setGeometry(
                rect.marginsRemoved(QMargins(1, 1, 0, 0)));
        }
        break;
        case Node::Type::VerticalContainer:
        case Node::Type::HorizontalContainer: {
            bool isVertical = this->type_ == Node::Type::VerticalContainer;

            // vars
            qreal minSize(48 * _scale);

            qreal totalFlex = std::max<qreal>(
                0.0001, this->getChildrensTotalFlex(isVertical));
            qreal totalSize = std::accumulate(
                this->children_.begin(), this->children_.end(), qreal(0),
                [=, this](int val, std::unique_ptr<Node> &node) {
                    return val + std::max<qreal>(
                                     this->getSize(isVertical) /
                                         std::max<qreal>(0.0001, totalFlex) *
                                         node->getFlex(isVertical),
                                     minSize);
                });

            totalSize = std::max<qreal>(0.0001, totalSize);

            qreal sizeMultiplier = this->getSize(isVertical) / totalSize;
            QRectF childRect = this->geometry_;

            // add spacing if reqested
            if (addSpacing)
            {
                qreal offset = std::min<qreal>(this->getSize(!isVertical) * 0.1,
                                               qreal(_scale * 24));

                // droprect left / above
                dropRects.emplace_back(
                    QRectF(this->geometry_.left(), this->geometry_.top(),
                           isVertical ? offset : this->geometry_.width(),
                           isVertical ? this->geometry_.height() : offset)
                        .toRect(),
                    Position(this, isVertical ? SplitDirection::Left
                                              : SplitDirection::Above));

                // droprect right / below
                if (isVertical)
                {
                    dropRects.emplace_back(
                        QRectF(this->geometry_.right() - offset,
                               this->geometry_.top(), offset,
                               this->geometry_.height())
                            .toRect(),
                        Position(this, SplitDirection::Right));
                }
                else
                {
                    dropRects.emplace_back(
                        QRectF(this->geometry_.left(),
                               this->geometry_.bottom() - offset,
                               this->geometry_.width(), offset)
                            .toRect(),
                        Position(this, SplitDirection::Below));
                }

                // shrink childRect
                if (isVertical)
                {
                    childRect.setLeft(childRect.left() + offset);
                    childRect.setRight(childRect.right() - offset);
                }
                else
                {
                    childRect.setTop(childRect.top() + offset);
                    childRect.setBottom(childRect.bottom() - offset);
                }
            }

            // iterate children
            auto pos = int(isVertical ? childRect.top() : childRect.left());
            for (std::unique_ptr<Node> &child : this->children_)
            {
                // set rect
                QRect rect = childRect.toRect();
                if (isVertical)
                {
                    rect.setTop(pos);
                    rect.setHeight(
                        std::max<qreal>(this->geometry_.height() / totalFlex *
                                            child->flexV_,
                                        minSize) *
                        sizeMultiplier);
                }
                else
                {
                    rect.setLeft(pos);
                    rect.setWidth(std::max<qreal>(this->geometry_.width() /
                                                      totalFlex * child->flexH_,
                                                  minSize) *
                                  sizeMultiplier);
                }

                if (child == this->children_.back())
                {
                    rect.setRight(childRect.right() - 1);
                    rect.setBottom(childRect.bottom() - 1);
                }

                child->geometry_ = rect;
                child->layout(addSpacing, _scale, dropRects, resizeRects);

                pos += child->getSize(isVertical);

                // add resize rect
                if (child != this->children_.front())
                {
                    QRectF r = isVertical ? QRectF(this->geometry_.left(),
                                                   child->geometry_.top() - 4,
                                                   this->geometry_.width(), 8)
                                          : QRectF(child->geometry_.left() - 4,
                                                   this->geometry_.top(), 8,
                                                   this->geometry_.height());
                    resizeRects.push_back(
                        ResizeRect(r.toRect(), child.get(), isVertical));
                }

                // normalize flex
                if (isVertical)
                {
                    child->flexV_ =
                        child->flexV_ / totalFlex * this->children_.size();
                    child->flexH_ = 1;
                }
                else
                {
                    child->flexH_ =
                        child->flexH_ / totalFlex * this->children_.size();
                    child->flexV_ = 1;
                }
            }
        }
        break;
    }
}

void SplitContainer::Node::clamp()
{
    this->flexH_ = std::max(0.0, this->flexH_);
    this->flexV_ = std::max(0.0, this->flexV_);
}

SplitContainer::Node::Type SplitContainer::Node::toContainerType(
    SplitDirection _dir)
{
    return _dir == SplitDirection::Left || _dir == SplitDirection::Right
               ? Type::HorizontalContainer
               : Type::VerticalContainer;
}

//
// DropOverlay
//

SplitContainer::DropOverlay::DropOverlay(SplitContainer *_parent)
    : QWidget(_parent)
    , mouseOverPoint_(-10000, -10000)
    , parent_(_parent)
{
    this->setMouseTracking(true);
    this->setAcceptDrops(true);
}

void SplitContainer::DropOverlay::setRects(
    std::vector<SplitContainer::DropRect> _rects)
{
    this->rects_ = std::move(_rects);
}

// pajlada::Signals::NoArgSignal dragEnded;

void SplitContainer::DropOverlay::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);

    //            painter.fillRect(this->rect(), QColor("#334"));

    bool foundMover = false;

    for (DropRect &rect : this->rects_)
    {
        if (!foundMover && rect.rect.contains(this->mouseOverPoint_))
        {
            painter.setBrush(getApp()->getThemes()->splits.dropPreview);
            painter.setPen(getApp()->getThemes()->splits.dropPreviewBorder);
            foundMover = true;
        }
        else
        {
            painter.setBrush(QColor(0, 0, 0, 0));
            painter.setPen(QColor(0, 0, 0, 0));
            // painter.setPen(getApp()->getThemes()->splits.dropPreviewBorder);
        }

        painter.drawRect(rect.rect);
    }
}

void SplitContainer::DropOverlay::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void SplitContainer::DropOverlay::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();

    this->mouseOverPoint_ = event->pos();
    this->update();
}

void SplitContainer::DropOverlay::dragLeaveEvent(QDragLeaveEvent * /*event*/)
{
    this->mouseOverPoint_ = QPoint(-10000, -10000);
    this->close();
    this->dragEnded.invoke();
}

void SplitContainer::DropOverlay::dropEvent(QDropEvent *event)
{
    Position *position = nullptr;
    for (DropRect &rect : this->rects_)
    {
        if (rect.rect.contains(this->mouseOverPoint_))
        {
            position = &rect.position;
            break;
        }
    }

    if (!position)
    {
        qCDebug(chatterinoWidget) << "No valid drop rectangle under cursor";
        return;
    }

    auto *draggedSplit = dynamic_cast<Split *>(event->source());
    if (!draggedSplit)
    {
        qCDebug(chatterinoWidget)
            << "Dropped something that wasn't a split onto a split container";
        return;
    }

    this->parent_->insertSplit(draggedSplit, {.position = *position});
    event->acceptProposedAction();

    this->mouseOverPoint_ = QPoint(-10000, -10000);
    this->close();
    this->dragEnded.invoke();
}

//
// ResizeHandle
//

void SplitContainer::ResizeHandle::setVertical(bool isVertical)
{
    this->setCursor(isVertical ? Qt::SplitVCursor : Qt::SplitHCursor);
    this->vertical_ = isVertical;
}

SplitContainer::ResizeHandle::ResizeHandle(SplitContainer *_parent)
    : QWidget(_parent)
    , parent(_parent)
{
    this->setMouseTracking(true);
    this->hide();
}

void SplitContainer::ResizeHandle::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setPen(QPen(getApp()->getThemes()->splits.resizeHandle, 2));

    painter.fillRect(this->rect(),
                     getApp()->getThemes()->splits.resizeHandleBackground);

    if (this->vertical_)
    {
        painter.drawLine(0, this->height() / 2, this->width(),
                         this->height() / 2);
    }
    else
    {
        painter.drawLine(this->width() / 2, 0, this->width() / 2,
                         this->height());
    }
}

void SplitContainer::ResizeHandle::mousePressEvent(QMouseEvent *event)
{
    this->isMouseDown_ = true;

    if (event->button() == Qt::RightButton)
    {
        this->resetFlex();
    }
}

void SplitContainer::ResizeHandle::mouseReleaseEvent(QMouseEvent * /*event*/)
{
    this->isMouseDown_ = false;
}

void SplitContainer::ResizeHandle::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->isMouseDown_)
    {
        return;
    }

    assert(node != nullptr);
    assert(node->parent_ != nullptr);

    const auto &siblings = node->parent_->getChildren();
    auto it = std::find_if(siblings.begin(), siblings.end(),
                           [this](const std::unique_ptr<Node> &n) {
                               return n.get() == this->node;
                           });

    assert(it != siblings.end());
    Node *before = siblings[it - siblings.begin() - 1].get();

    QPoint topLeft =
        this->parent->mapToGlobal(before->geometry_.topLeft().toPoint());
    QPoint bottomRight = this->parent->mapToGlobal(
        this->node->geometry_.bottomRight().toPoint());

    int globalX = topLeft.x() > event->globalX()
                      ? topLeft.x()
                      : (bottomRight.x() < event->globalX() ? bottomRight.x()
                                                            : event->globalX());
    int globalY = topLeft.y() > event->globalY()
                      ? topLeft.y()
                      : (bottomRight.y() < event->globalY() ? bottomRight.y()
                                                            : event->globalY());

    QPoint mousePoint(globalX, globalY);

    if (this->vertical_)
    {
        qreal totalFlexV = this->node->flexV_ + before->flexV_;
        before->flexV_ = totalFlexV * (mousePoint.y() - topLeft.y()) /
                         (bottomRight.y() - topLeft.y());
        this->node->flexV_ = totalFlexV - before->flexV_;

        this->parent->layout();

        // move handle
        this->move(this->x(), int(before->geometry_.bottom() - 4));
    }
    else
    {
        qreal totalFlexH = this->node->flexH_ + before->flexH_;
        before->flexH_ = totalFlexH * (mousePoint.x() - topLeft.x()) /
                         (bottomRight.x() - topLeft.x());
        this->node->flexH_ = totalFlexH - before->flexH_;

        this->parent->layout();

        // move handle
        this->move(int(before->geometry_.right() - 4), this->y());
    }
}

void SplitContainer::ResizeHandle::mouseDoubleClickEvent(QMouseEvent *event)
{
    event->accept();

    this->resetFlex();
}

void SplitContainer::ResizeHandle::resetFlex()
{
    for (const auto &sibling : this->node->getParent()->getChildren())
    {
        sibling->flexH_ = 1;
        sibling->flexV_ = 1;
    }

    this->parent->layout();
}

}  // namespace chatterino
