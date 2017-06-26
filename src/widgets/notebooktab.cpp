#include "widgets/notebooktab.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"
#include "widgets/notebook.hpp"

#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
    , colorScheme(notebook->colorScheme)
    , _posAnimation(this, "pos")
    , _notebook(notebook)
{
    this->calcSize();
    this->setAcceptDrops(true);

    _posAnimation.setEasingCurve(QEasingCurve(QEasingCurve::InCubic));

    this->_hideXConnection = SettingsManager::getInstance().hideTabX.valueChanged.connect(
        boost::bind(&NotebookTab::hideTabXChanged, this, _1));

    this->setMouseTracking(true);
}

NotebookTab::~NotebookTab()
{
    this->_hideXConnection.disconnect();
}

void NotebookTab::calcSize()
{
    if (SettingsManager::getInstance().hideTabX.get()) {
        resize(fontMetrics().width(_title) + 8, 24);
    } else {
        resize(fontMetrics().width(_title) + 8 + 24, 24);
    }

    if (parent() != nullptr) {
        ((Notebook *)parent())->performLayout(true);
    }
}

const QString &NotebookTab::getTitle() const
{
    return _title;
}

void NotebookTab::setTitle(const QString &title)
{
    _title = title;
}

bool NotebookTab::getSelected()
{
    return _selected;
}

void NotebookTab::setSelected(bool value)
{
    _selected = value;
    update();
}

NotebookTab::HighlightStyle NotebookTab::getHighlightStyle() const
{
    return _highlightStyle;
}

void NotebookTab::setHighlightStyle(HighlightStyle style)
{
    _highlightStyle = style;
    update();
}

QRect NotebookTab::getDesiredRect() const
{
    return QRect(_posAnimationDesired, size());
}

void NotebookTab::hideTabXChanged(bool)
{
    calcSize();
    update();
}

void NotebookTab::moveAnimated(QPoint pos, bool animated)
{
    _posAnimationDesired = pos;

    if ((window() != nullptr && !window()->isVisible()) || !animated || _posAnimated == false) {
        move(pos);

        _posAnimated = true;
        return;
    }

    if (_posAnimation.endValue() == pos) {
        return;
    }

    _posAnimation.stop();
    _posAnimation.setDuration(75);
    _posAnimation.setStartValue(this->pos());
    _posAnimation.setEndValue(pos);
    _posAnimation.start();
}

void NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor fg = QColor(0, 0, 0);

    if (_selected) {
        painter.fillRect(rect(), this->colorScheme.TabSelectedBackground);
        fg = this->colorScheme.TabSelectedText;
    } else if (_mouseOver) {
        painter.fillRect(rect(), this->colorScheme.TabHoverBackground);
        fg = this->colorScheme.TabHoverText;
    } else if (_highlightStyle == HighlightHighlighted) {
        painter.fillRect(rect(), this->colorScheme.TabHighlightedBackground);
        fg = this->colorScheme.TabHighlightedText;
    } else if (_highlightStyle == HighlightNewMessage) {
        painter.fillRect(rect(), this->colorScheme.TabNewMessageBackground);
        fg = this->colorScheme.TabHighlightedText;
    } else {
        painter.fillRect(rect(), this->colorScheme.TabBackground);
        fg = this->colorScheme.TabText;
    }

    painter.setPen(fg);

    QRect rect(0, 0, width() - (SettingsManager::getInstance().hideTabX.get() ? 0 : 16), height());

    painter.drawText(rect, _title, QTextOption(Qt::AlignCenter));

    if (!SettingsManager::getInstance().hideTabX.get() && (_mouseOver || _selected)) {
        if (_mouseOverX) {
            painter.fillRect(getXRect(), QColor(0, 0, 0, 64));

            if (_mouseDownX) {
                painter.fillRect(getXRect(), QColor(0, 0, 0, 64));
            }
        }

        painter.drawLine(getXRect().topLeft() + QPoint(4, 4),
                         getXRect().bottomRight() + QPoint(-4, -4));
        painter.drawLine(getXRect().topRight() + QPoint(-4, 4),
                         getXRect().bottomLeft() + QPoint(4, -4));
    }
}

void NotebookTab::mousePressEvent(QMouseEvent *event)
{
    _mouseDown = true;
    _mouseDownX = getXRect().contains(event->pos());

    update();

    _notebook->select(page);
}

void NotebookTab::mouseReleaseEvent(QMouseEvent *event)
{
    _mouseDown = false;

    if (!SettingsManager::getInstance().hideTabX.get() && _mouseDownX &&
        getXRect().contains(event->pos())) {
        _mouseDownX = false;

        _notebook->removePage(page);
    } else {
        update();
    }
}

void NotebookTab::enterEvent(QEvent *)
{
    _mouseOver = true;

    update();
}

void NotebookTab::leaveEvent(QEvent *)
{
    _mouseOverX = _mouseOver = false;

    update();
}

void NotebookTab::dragEnterEvent(QDragEnterEvent *)
{
    _notebook->select(page);
}

void NotebookTab::mouseMoveEvent(QMouseEvent *event)
{
    bool overX = getXRect().contains(event->pos());

    if (overX != _mouseOverX) {
        _mouseOverX = overX && !SettingsManager::getInstance().hideTabX.get();

        update();
    }

    if (_mouseDown && !getDesiredRect().contains(event->pos())) {
        QPoint relPoint = mapToParent(event->pos());

        int index;
        NotebookPage *clickedPage = _notebook->tabAt(relPoint, index);

        if (clickedPage != nullptr && clickedPage != page) {
            _notebook->rearrangePage(clickedPage, index);
        }
    }
}

void NotebookTab::load(const boost::property_tree::ptree &tree)
{
    // Load tab title
    try {
        setTitle(QString::fromStdString(tree.get<std::string>("title")));
    } catch (boost::property_tree::ptree_error) {
    }
}

boost::property_tree::ptree NotebookTab::save()
{
    boost::property_tree::ptree tree;

    tree.put("title", getTitle().toStdString());

    return tree;
}

}  // namespace widgets
}  // namespace chatterino
