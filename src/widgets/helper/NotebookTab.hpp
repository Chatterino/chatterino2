#pragma once

#include "common/Common.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/helper/RippleEffectButton.hpp"

#include <QMenu>
#include <QPropertyAnimation>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/connection.hpp>

namespace chatterino {

#define NOTEBOOK_TAB_HEIGHT 28

// class Notebook;
class Notebook;
class SplitContainer;

class NotebookTab : public RippleEffectButton
{
    Q_OBJECT

public:
    explicit NotebookTab(Notebook *_notebook);

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

    void setHighlightState(HighlightState style);

    void moveAnimated(QPoint pos, bool animated = true);

    QRect getDesiredRect() const;
    void hideTabXChanged(bool);

protected:
    virtual void themeChangedEvent() override;

    virtual void paintEvent(QPaintEvent *) override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

    virtual void dragEnterEvent(QDragEnterEvent *event) override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;

    QPropertyAnimation positionChangedAnimation_;
    bool positionChangedAnimationRunning_ = false;
    QPoint positionAnimationDesiredPoint_;

    Notebook *notebook_;

    QString customTitle_;
    QString defaultTitle_;

    bool selected_ = false;
    bool mouseOver_ = false;
    bool mouseDown_ = false;
    bool mouseOverX_ = false;
    bool mouseDownX_ = false;

    bool hasXButton();
    bool shouldDrawXButton();

    HighlightState highlightState_ = HighlightState::None;

    QMenu menu_;

    QRect getXRect();
    void titleUpdated();
};

}  // namespace chatterino
