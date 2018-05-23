#pragma once

#include "common.hpp"
#include "widgets/basewidget.hpp"

#include <QMenu>
#include <QPropertyAnimation>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/connection.hpp>

namespace chatterino {
namespace widgets {

#define NOTEBOOK_TAB_HEIGHT 28

// class Notebook;
class Notebook2;
class SplitContainer;

class NotebookTab2 : public BaseWidget
{
    Q_OBJECT

public:
    explicit NotebookTab2(Notebook2 *_notebook);

    void updateSize();

    QWidget *page;

    const QString &getTitle() const;
    void setTitle(const QString &newTitle);
    bool isSelected() const;
    void setSelected(bool value);

    void setHighlightState(HighlightState style);

    void moveAnimated(QPoint pos, bool animated = true);

    QRect getDesiredRect() const;
    void hideTabXChanged(bool);

protected:
    virtual void themeRefreshEvent() override;

    virtual void paintEvent(QPaintEvent *) override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

    virtual void dragEnterEvent(QDragEnterEvent *event) override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
    std::vector<pajlada::Signals::ScopedConnection> managedConnections;

    QPropertyAnimation positionChangedAnimation;
    bool positionChangedAnimationRunning = false;
    QPoint positionAnimationDesiredPoint;

    Notebook2 *notebook;

    QString title;

public:
    bool useDefaultTitle = true;

private:
    bool selected = false;
    bool mouseOver = false;
    bool mouseDown = false;
    bool mouseOverX = false;
    bool mouseDownX = false;

    bool hasXButton();
    bool shouldDrawXButton();

    HighlightState highlightState = HighlightState::None;

    QMenu menu;

    QRect getXRect();
};

}  // namespace widgets
}  // namespace chatterino
