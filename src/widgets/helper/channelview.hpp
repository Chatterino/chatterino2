#pragma once

#include "channel.hpp"
#include "messages/image.hpp"
#include "messages/layouts/messagelayout.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/messageelement.hpp"
#include "messages/selection.hpp"
#include "widgets/accountpopup.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"
#include "widgets/scrollbar.hpp"

#include <QPaintEvent>
#include <QScroller>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <pajlada/signals/signal.hpp>

#include <unordered_set>

namespace chatterino {
namespace widgets {

class ChannelView : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChannelView(BaseWidget *parent = nullptr);
    virtual ~ChannelView();

    void queueUpdate();
    Scrollbar &getScrollBar();
    QString getSelectedText();
    bool hasSelection();
    void clearSelection();
    void setEnableScrollingToBottom(bool);
    bool getEnableScrollingToBottom() const;
    void setOverrideFlags(boost::optional<messages::MessageElement::Flags> value);
    const boost::optional<messages::MessageElement::Flags> &getOverrideFlags() const;
    void pause(int msecTimeout);
    void updateLastReadMessage();

    void setChannel(ChannelPtr channel);
    messages::LimitedQueueSnapshot<messages::MessageLayoutPtr> getMessagesSnapshot();
    void layoutMessages();

    void clearMessages();

    pajlada::Signals::Signal<QMouseEvent *> mouseDown;
    pajlada::Signals::NoArgSignal selectionChanged;
    pajlada::Signals::NoArgSignal highlightedMessageReceived;
    pajlada::Signals::Signal<const messages::Link &> linkClicked;

protected:
    void themeRefreshEvent() override;

    void resizeEvent(QResizeEvent *) override;

    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *event) override;

    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void handleLinkClick(QMouseEvent *event, const messages::Link &link,
                         messages::MessageLayout *layout);

    bool tryGetMessageAt(QPoint p, std::shared_ptr<messages::MessageLayout> &message,
                         QPoint &relativePos, int &index);

private:
    QTimer *layoutCooldown;
    bool layoutQueued;

    QTimer updateTimer;
    bool updateQueued = false;
    bool messageWasAdded = false;
    bool paused = false;
    QTimer pauseTimeout;
    boost::optional<messages::MessageElement::Flags> overrideFlags;
    messages::MessageLayoutPtr lastReadMessage;

    messages::LimitedQueueSnapshot<messages::MessageLayoutPtr> snapshot;

    void detachChannel();
    void actuallyLayoutMessages();

    void drawMessages(QPainter &painter);
    void setSelection(const messages::SelectionItem &start, const messages::SelectionItem &end);
    messages::MessageElement::Flags getFlags() const;

    ChannelPtr channel;

    Scrollbar scrollBar;
    RippleEffectLabel *goToBottom;

    // This variable can be used to decide whether or not we should render the "Show latest
    // messages" button
    bool showingLatestMessages = true;
    bool enableScrollingToBottom = true;

    AccountPopupWidget userPopupWidget;
    bool onlyUpdateEmotes = false;

    // Mouse event variables
    bool isMouseDown = false;
    QPointF lastPressPosition;

    messages::Selection selection;
    bool selecting = false;

    messages::LimitedQueue<messages::MessageLayoutPtr> messages;

    pajlada::Signals::Connection messageAppendedConnection;
    pajlada::Signals::Connection messageAddedAtStartConnection;
    pajlada::Signals::Connection messageRemovedConnection;
    pajlada::Signals::Connection messageReplacedConnection;
    pajlada::Signals::Connection repaintGifsConnection;
    pajlada::Signals::Connection layoutConnection;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections;

    std::unordered_set<std::shared_ptr<messages::MessageLayout>> messagesOnScreen;

private slots:
    void wordFlagsChanged()
    {
        layoutMessages();
        update();
    }
};

}  // namespace widgets
}  // namespace chatterino
