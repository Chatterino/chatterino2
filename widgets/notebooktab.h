#ifndef NOTEBOOKTAB_H
#define NOTEBOOKTAB_H

#include <QPropertyAnimation>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>

namespace chatterino {
namespace widgets {

class Notebook;
class NotebookPage;

class NotebookTab : public QWidget
{
    Q_OBJECT

public:
    enum HighlightStyle {
        HighlightNone,
        HighlightHighlighted,
        HighlightNewMessage
    };

    explicit NotebookTab(Notebook *notebook);
    ~NotebookTab();

    void calcSize();

    NotebookPage *page;

    const QString &
    getText() const
    {
        return this->text;
    }

    void
    setText(const QString &text)
    {
        this->text = text;
    }

    bool
    getSelected()
    {
        return this->selected;
    }

    void
    setSelected(bool value)
    {
        this->selected = value;
        update();
    }

    HighlightStyle
    getHighlightStyle() const
    {
        return this->highlightStyle;
    }

    void
    setHighlightStyle(HighlightStyle style)
    {
        this->highlightStyle = style;
        update();
    }

    void moveAnimated(QPoint pos, bool animated = true);

public:
    QRect
    getDesiredRect() const
    {
        return QRect(posAnimationDesired, this->size());
    }

protected:
    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPropertyAnimation posAnimation;
    bool posAnimated;
    QPoint posAnimationDesired;

    Notebook *notebook;

    QString text;

    bool selected;
    bool mouseOver;
    bool mouseDown;
    bool mouseOverX;
    bool mouseDownX;

    HighlightStyle highlightStyle;

    QRect
    getXRect()
    {
        return QRect(this->width() - 20, 4, 16, 16);
    }

private slots:
    void
    hideTabXChanged(bool)
    {
        calcSize();
        update();
    }

public:
    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
};

}  // namespace widgets
}  // namespace chatterino

#endif  // NOTEBOOKTAB_H
