#include "widgets/scrollbar.h"
#include "colorscheme.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>

#define MIN_THUMB_HEIGHT 10

namespace chatterino {
namespace widgets {

ScrollBar::ScrollBar(QWidget *widget)
    : QWidget(widget)
    , _mutex()
    , _currentValueAnimation(this, "currentValue")
    , _highlights(nullptr)
    , _mouseOverIndex(-1)
    , _mouseDownIndex(-1)
    , _lastMousePosition()
    , _buttonHeight(16)
    , _trackHeight(100)
    , _thumbRect()
    , _maximum()
    , _minimum()
    , _largeChange()
    , _smallChange()
    , _desiredValue()
    , _currentValue()
    , _currentValueChanged()
{
    resize(16, 100);

    _currentValueAnimation.setDuration(250);
    _currentValueAnimation.setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));

    setMouseTracking(true);
}

ScrollBar::~ScrollBar()
{
    auto highlight = _highlights;

    while (highlight != NULL) {
        auto tmp = highlight->next;
        delete highlight;
        highlight = tmp;
    }
}

void ScrollBar::removeHighlightsWhere(std::function<bool(ScrollBarHighlight &)> func)
{
    _mutex.lock();

    ScrollBarHighlight *last = NULL;
    ScrollBarHighlight *current = _highlights;

    while (current != NULL) {
        if (func(*current)) {
            if (last == NULL) {
                _highlights = current->next;
            } else {
                last->next = current->next;
            }

            auto oldCurrent = current;

            current = current->next;
            last = current;

            delete oldCurrent;
        }
    }

    _mutex.unlock();
}

void ScrollBar::addHighlight(ScrollBarHighlight *highlight)
{
    _mutex.lock();

    if (_highlights == NULL) {
        _highlights = highlight;
    } else {
        highlight->next = _highlights->next;
        _highlights->next = highlight;
    }

    _mutex.unlock();
}

void ScrollBar::scrollToBottom()
{
    this->setDesiredValue(this->_maximum - this->getLargeChange());
}

bool ScrollBar::isAtBottom() const
{
    return this->getCurrentValue() == this->getMaximum() - this->getLargeChange();
}

void ScrollBar::setMaximum(qreal value)
{
    _maximum = value;

    updateScroll();
}

void ScrollBar::setMinimum(qreal value)
{
    _minimum = value;

    updateScroll();
}

void ScrollBar::setLargeChange(qreal value)
{
    _largeChange = value;

    updateScroll();
}

void ScrollBar::setSmallChange(qreal value)
{
    _smallChange = value;

    updateScroll();
}

void ScrollBar::setDesiredValue(qreal value, bool animated)
{
    value = std::max(_minimum, std::min(_maximum - _largeChange, value));

    if (_desiredValue != value) {
        if (animated) {
            _currentValueAnimation.stop();
            _currentValueAnimation.setStartValue(_currentValue);

            _currentValueAnimation.setEndValue(value);
            _currentValueAnimation.start();
        } else {
            if (_currentValueAnimation.state() != QPropertyAnimation::Running) {
                // currentValueAnimation.stop();

                setCurrentValue(value);
            }
        }
    }

    _desiredValue = value;
}

qreal ScrollBar::getMaximum() const
{
    return _maximum;
}

qreal ScrollBar::getMinimum() const
{
    return _minimum;
}

qreal ScrollBar::getLargeChange() const
{
    return _largeChange;
}

qreal ScrollBar::getSmallChange() const
{
    return _smallChange;
}

qreal ScrollBar::getDesiredValue() const
{
    return _desiredValue;
}

qreal ScrollBar::getCurrentValue() const
{
    return _currentValue;
}

boost::signals2::signal<void()> &ScrollBar::getCurrentValueChanged()
{
    return _currentValueChanged;
}

void ScrollBar::setCurrentValue(qreal value)
{
    value = std::max(_minimum, std::min(_maximum - _largeChange, value));

    if (_currentValue != value) {
        _currentValue = value;

        updateScroll();
        _currentValueChanged();

        update();
    }
}

void ScrollBar::printCurrentState(const QString &prefix) const
{
    qDebug() << prefix                                         //
             << "Current value: " << this->getCurrentValue()   //
             << ". Maximum: " << this->getMaximum()            //
             << ". Minimum: " << this->getMinimum()            //
             << ". Large change: " << this->getLargeChange();  //
}

void ScrollBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), ColorScheme::getInstance().ScrollbarBG);

    painter.fillRect(QRect(0, 0, width(), _buttonHeight), QColor(255, 0, 0));
    painter.fillRect(QRect(0, height() - _buttonHeight, width(), _buttonHeight), QColor(255, 0, 0));

    painter.fillRect(_thumbRect, QColor(0, 255, 255));

    //    ScrollBarHighlight *highlight = highlights;

    _mutex.lock();

    //    do {
    //        painter.fillRect();
    //    } while ((highlight = highlight->next()) != NULL);

    _mutex.unlock();
}

void ScrollBar::mouseMoveEvent(QMouseEvent *event)
{
    if (_mouseDownIndex == -1) {
        int y = event->pos().y();

        auto oldIndex = _mouseOverIndex;

        if (y < _buttonHeight) {
            _mouseOverIndex = 0;
        } else if (y < _thumbRect.y()) {
            _mouseOverIndex = 1;
        } else if (_thumbRect.contains(2, y)) {
            _mouseOverIndex = 2;
        } else if (y < height() - _buttonHeight) {
            _mouseOverIndex = 3;
        } else {
            _mouseOverIndex = 4;
        }

        if (oldIndex != _mouseOverIndex) {
            update();
        }
    } else if (_mouseDownIndex == 2) {
        int delta = event->pos().y() - _lastMousePosition.y();

        setDesiredValue(_desiredValue + (qreal)delta / _trackHeight * _maximum);
    }

    _lastMousePosition = event->pos();
}

void ScrollBar::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < _buttonHeight) {
        _mouseDownIndex = 0;
    } else if (y < _thumbRect.y()) {
        _mouseDownIndex = 1;
    } else if (_thumbRect.contains(2, y)) {
        _mouseDownIndex = 2;
    } else if (y < height() - _buttonHeight) {
        _mouseDownIndex = 3;
    } else {
        _mouseDownIndex = 4;
    }
}

void ScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < _buttonHeight) {
        if (_mouseDownIndex == 0) {
            setDesiredValue(_desiredValue - _smallChange, true);
        }
    } else if (y < _thumbRect.y()) {
        if (_mouseDownIndex == 1) {
            setDesiredValue(_desiredValue - _smallChange, true);
        }
    } else if (_thumbRect.contains(2, y)) {
        // do nothing
    } else if (y < height() - _buttonHeight) {
        if (_mouseDownIndex == 3) {
            setDesiredValue(_desiredValue + _smallChange, true);
        }
    } else {
        if (_mouseDownIndex == 4) {
            setDesiredValue(_desiredValue + _smallChange, true);
        }
    }

    _mouseDownIndex = -1;
    update();
}

void ScrollBar::leaveEvent(QEvent *)
{
    _mouseOverIndex = -1;

    update();
}

void ScrollBar::updateScroll()
{
    _trackHeight = height() - _buttonHeight - _buttonHeight - MIN_THUMB_HEIGHT - 1;

    _thumbRect = QRect(0, (int)(_currentValue / _maximum * _trackHeight) + 1 + _buttonHeight,
                       width(), (int)(_largeChange / _maximum * _trackHeight) + MIN_THUMB_HEIGHT);

    update();
}

}  // namespace widgets
}  // namespace chatterino
