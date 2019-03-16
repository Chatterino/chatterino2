#pragma once

#include "MessageView.Pauser.hpp"
#include "Room.hpp"
#include "ab/BaseWidget.hpp"
#include "ab/util/FlagsEnum.hpp"
#include "messages/MessageContainer.hpp"
#include "messages/layouts/MessageLayout.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "ui/MessageView.Selector.hpp"
#include "ui/UiFwd.hpp"

#include <QPaintEvent>
#include <QScroller>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace chatterino
{
    enum class HighlightState;

    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    enum class MessageFlag : uint16_t;
    using MessageFlags = ab::FlagsEnum<MessageFlag>;

    class MessageLayout;
    using MessageLayoutPtr = std::shared_ptr<MessageLayout>;  // todo remove

    enum class MessageElementFlag;
    using MessageElementFlags = ab::FlagsEnum<MessageElementFlag>;

    struct Link;
    class MessageLayoutElement;
}  // namespace chatterino

namespace chatterino::ui
{
    class Scrollbar;
    class EffectLabel;

    class MessageView final : public ab::BaseWidget
    {
        Q_OBJECT

        using LayoutContainerType = std::deque<std::unique_ptr<MessageLayout>>;

    public:
        explicit MessageView(QWidget* parent = nullptr);

        [[nodiscard]] Scrollbar& scrollbar();

        [[nodiscard]] QString selectedText();
        [[nodiscard]] bool hasSelection();
        void clearSelection();

        void setEnableScrollingToBottom(bool);
        [[nodiscard]] bool enableScrollingToBottom() const;

        void setOverrideFlags(std::optional<MessageElementFlags>);
        [[nodiscard]] const std::optional<MessageElementFlags>& overrideFlags()
            const;

        [[nodiscard]] bool pausable() const;
        void setPausable(bool);

        [[nodiscard]] std::shared_ptr<MessageContainer> container();
        void setContainer(const std::shared_ptr<MessageContainer>& messages_);

        void queueUpdate();
        void queueLayout();
        void clearMessages();
        void showUserInfoPopup(const QString& userName);
        void updateLastReadMessage();

    signals:
        void mouseDown(QMouseEvent*);
        void selectionChanged();
        void tabHighlightRequested(HighlightState);
        void liveStatusChanged();
        void linkClicked(const Link&);
        void joinToChannel(QString);

    protected:
        // void themeChangedEvent() override;
        // void scaleChangedEvent(float scale) override;

        void resizeEvent(QResizeEvent*) override;

        void paintEvent(QPaintEvent*) override;
        void wheelEvent(QWheelEvent*) override;

        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;

        void mouseMoveEvent(QMouseEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void mouseDoubleClickEvent(QMouseEvent*) override;

        void hideEvent(QHideEvent*) override;

        void handleLinkClick(QMouseEvent*, const Link&, MessageLayout*);

    private:
        struct MessageAt
        {
            MessageLayout& layout;
            QPoint relativePos;
            int index;
        };

        [[nodiscard]] std::optional<MessageAt> messageAt(QPoint p);

        void initializeLayout();
        void initializeSignals();

        void messagesInserted(const MessageContainer::Insertion&);
        void messagesErased(const MessageContainer::Erasure&);

        void messageClicked(MessageAt&, QMouseEvent*);
        void messageDoubleClicked(MessageAt&, QMouseEvent*);
        void messagePressed(MessageAt&, QMouseEvent*);
        void pressedBelowMessages(QMouseEvent*);

        void performLayout(bool causedByScollbar = false);
        void layoutVisibleMessages(const LayoutContainerType& messages);
        void updateScrollbar(
            const LayoutContainerType& messages, bool causedByScrollbar);

        void drawMessages(QPainter& painter);
        [[nodiscard]] MessageElementFlags flags() const;
        void selectWholeMessage(MessageLayout& layout, int& messageIndex);

        /// @result start index, end index
        [[nodiscard]] std::pair<int, int> wordBounds(MessageLayout* layout,
            const MessageLayoutElement* element, const QPoint& relativePos);

        void handleMouseClick(QMouseEvent* event,
            const MessageLayoutElement* hoverLayoutElement,
            MessageLayout* layout);
        void addContextMenuItems(
            const MessageLayoutElement* hoveredElement, MessageLayout* layout);
        [[nodiscard]] int layoutWidth() const;

        QTimer* layoutCooldown_;
        bool layoutQueued_;

        QTimer updateTimer_;
        bool updateQueued_ = false;
        bool messageWasAdded_ = false;
        bool lastMessageHasAlternateBackground_ = false;
        bool lastMessageHasAlternateBackgroundReverse_ = true;

        int pauseScrollOffset_ = 0;

        std::optional<MessageElementFlags> overrideFlags_;
        void* lastReadMessage_{};

        Scrollbar* scrollBar_{};
        QWidget* goToBottom_{};

        // This variable can be used to decide whether or not we should render
        // the "Show latest messages" button
        bool showingLatestMessages_ = true;
        bool enableScrollingToBottom_ = true;

        bool onlyUpdateEmotes_ = false;

        // Mouse event variables
        bool isMouseDown_ = false;
        bool isRightMouseDown_ = false;
        bool isDoubleClick_ = false;
        bool nextClickIsTriple = false;
        QPointF lastPressPosition_;
        QPointF lastRightPressPosition_;
        QTimer* clickTimer_;

        std::shared_ptr<MessageContainer> messages_;
        LayoutContainerType layouts_;  // std::deque

        std::vector<QMetaObject::Connection> channelConnections_;
        std::unordered_set<MessageLayout*> messagesOnScreen_;

        bool smoothScrolling_{};

        ThemexD* theme{};

        Pauser pauser_;
        Selector selector_;

        static constexpr int leftPadding = 8;       // TODO: remove
        static constexpr int scrollbarPadding = 8;  // TODO: remove

    private slots:
        void wordFlagsChanged()
        {
            this->queueLayout();
            this->update();
        }
    };
}  // namespace chatterino::ui
