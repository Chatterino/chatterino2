#pragma once

#include <QPropertyAnimation>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>
#include <boost/signals2.hpp>
#include <boost/signals2/connection.hpp>

namespace chatterino {

class ColorScheme;

namespace widgets {

class Notebook;
class NotebookPage;

class NotebookTab : public QWidget
{
    Q_OBJECT

public:
    enum HighlightStyle { HighlightNone, HighlightHighlighted, HighlightNewMessage };

    explicit NotebookTab(Notebook *_notebook);
    ~NotebookTab();

    ColorScheme &colorScheme;

    void calcSize();

    NotebookPage *page;

    const QString &getTitle() const;
    void setTitle(const QString &newTitle);
    bool isSelected() const;
    void setSelected(bool value);

    HighlightStyle getHighlightStyle() const;
    void setHighlightStyle(HighlightStyle style);

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
    boost::signals2::connection hideXConnection;

    QPropertyAnimation positionChangedAnimation;
    bool positionChangedAnimationRunning = false;
    QPoint positionAnimationDesiredPoint;

    Notebook *notebook;

    QString title = "<no title>";

    bool selected = false;
    bool mouseOver = false;
    bool mouseDown = false;
    bool mouseOverX = false;
    bool mouseDownX = false;

    HighlightStyle highlightStyle = HighlightStyle::HighlightNone;

    QRect getXRect()
    {
        return QRect(this->width() - 20, 4, 16, 16);
    }

public:
    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
};

}  // namespace widgets
}  // namespace chatterino
