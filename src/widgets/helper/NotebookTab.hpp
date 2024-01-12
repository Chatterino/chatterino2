#pragma once

#include "common/Common.hpp"
#include "widgets/helper/Button.hpp"
#include "widgets/Notebook.hpp"

#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QMenu>
#include <QPropertyAnimation>

namespace chatterino {

#define NOTEBOOK_TAB_HEIGHT 28

class SplitContainer;

class NotebookTab : public Button
{
    Q_OBJECT

public:
    explicit NotebookTab(Notebook *notebook);

    void updateSize();

    QWidget *page{};

    void setCustomTitle(const QString &title);
    void resetCustomTitle();
    bool hasCustomTitle() const;
    const QString &getCustomTitle() const;
    void setDefaultTitle(const QString &title);
    const QString &getDefaultTitle() const;
    const QString &getTitle() const;

    bool isSelected() const;
    void setSelected(bool value);

    void setInLastRow(bool value);
    void setTabLocation(NotebookTabLocation location);

    /**
     * @brief Sets the live status of this tab
     *
     * Returns true if the live status was changed, false if nothing changed.
     **/
    bool setLive(bool isLive);

    /**
     * @brief Returns true if any split in this tab is live
     **/
    bool isLive() const;

    void setHighlightState(HighlightState style);
    HighlightState highlightState() const;

    void setHighlightsEnabled(const bool &newVal);
    bool hasHighlightsEnabled() const;

    void moveAnimated(QPoint pos, bool animated = true);

    QRect getDesiredRect() const;
    void hideTabXChanged();

    void growWidth(int width);
    int normalTabWidth();

protected:
    void themeChangedEvent() override;

    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    /// This exists as an alias to its base classes update, and is virtual
    /// to allow for mocking
    virtual void update();

private:
    void showRenameDialog();

    bool hasXButton();
    bool shouldDrawXButton();
    QRect getXRect();
    void titleUpdated();

    QPropertyAnimation positionChangedAnimation_;
    bool positionChangedAnimationRunning_ = false;
    QPoint positionAnimationDesiredPoint_;

    Notebook *notebook_;

    QString customTitle_;
    QString defaultTitle_;

    bool selected_{};
    bool mouseOver_{};
    bool mouseDown_{};
    bool mouseOverX_{};
    bool mouseDownX_{};
    bool isInLastRow_{};
    int mouseWheelDelta_ = 0;
    NotebookTabLocation tabLocation_ = NotebookTabLocation::Top;

    HighlightState highlightState_ = HighlightState::None;
    bool highlightEnabled_ = true;
    QAction *highlightNewMessagesAction_;

    bool isLive_{};

    int growWidth_ = 0;

    QMenu menu_;

    pajlada::Signals::SignalHolder managedConnections_;
};

}  // namespace chatterino
