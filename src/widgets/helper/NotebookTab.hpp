#pragma once

#include "common/Common.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/helper/Button.hpp"

#include <QColor>
#include <QMenu>
#include <QPropertyAnimation>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/connection.hpp>

namespace chatterino {

#define NOTEBOOK_TAB_HEIGHT 28

// class Notebook;
class Notebook;
class SplitContainer;

class NotebookTab : public Button
{
    Q_OBJECT

public:
    explicit NotebookTab(Notebook *notebook);

    void updateSize();

    QWidget *page;

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

    void setLive(bool isLive);
    void setHighlightState(HighlightState style);
    void setHighlightsEnabled(const bool &newVal);
    bool hasHighlightsEnabled() const;
    void setHighlightColor(std::shared_ptr<QColor> color);

    void moveAnimated(QPoint pos, bool animated = true);

    QRect getDesiredRect() const;
    void hideTabXChanged();

protected:
    virtual void themeChangedEvent() override;

    virtual void paintEvent(QPaintEvent *) override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

    virtual void dragEnterEvent(QDragEnterEvent *event) override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

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

    HighlightState highlightState_ = HighlightState::None;
    bool highlightEnabled_ = true;
    QAction *highlightNewMessagesAction_;
    std::shared_ptr<QColor> highlightColor_;

    bool isLive_{};

    QMenu menu_;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
};

}  // namespace chatterino
