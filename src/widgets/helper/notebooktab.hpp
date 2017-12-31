#pragma once

#include "common.hpp"
#include "widgets/basewidget.hpp"

#include <QMenu>
#include <QPropertyAnimation>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/connection.hpp>

namespace chatterino {

namespace widgets {

class Notebook;
class SplitContainer;

class NotebookTab : public BaseWidget
{
    Q_OBJECT

    const std::string uuid;
    const std::string settingRoot;

public:
    explicit NotebookTab(Notebook *_notebook, const std::string &_uuid);

    void calcSize();

    SplitContainer *page;

    QString getTitle() const;
    void setTitle(const QString &newTitle);
    bool isSelected() const;
    void setSelected(bool value);

    void setHighlightState(HighlightState style);

    void moveAnimated(QPoint pos, bool animated = true);

    QRect getDesiredRect() const;
    void hideTabXChanged(bool);

protected:
    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

private:
    std::vector<pajlada::Signals::ScopedConnection> managedConnections;

    QPropertyAnimation positionChangedAnimation;
    bool positionChangedAnimationRunning = false;
    QPoint positionAnimationDesiredPoint;

    Notebook *notebook;

    pajlada::Settings::Setting<std::string> title;

public:
    pajlada::Settings::Setting<bool> useDefaultBehaviour;

private:
    bool selected = false;
    bool mouseOver = false;
    bool mouseDown = false;
    bool mouseOverX = false;
    bool mouseDownX = false;

    HighlightState highlightState = HighlightState::None;

    QMenu menu;

    QRect getXRect()
    {
        float scale = this->getDpiMultiplier();
        return QRect(this->width() - static_cast<int>(20 * scale), static_cast<int>(4 * scale),
                     static_cast<int>(16 * scale), static_cast<int>(16 * scale));
    }
};

}  // namespace widgets
}  // namespace chatterino
