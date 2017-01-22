#include "widgets/notebooktab.h"
#include "colorscheme.h"
#include "settings/settings.h"
#include "widgets/notebook.h"

#include <QPainter>

namespace chatterino {
namespace widgets {

NotebookTab::NotebookTab(Notebook *notebook)
    : QWidget(notebook)
    , notebook(notebook)
    , text("<no title>")
    , selected(false)
    , mouseOver(false)
    , mouseDown(false)
    , mouseOverX(false)
    , mouseDownX(false)
    , highlightStyle(HighlightNone)
{
    calcSize();

    setAcceptDrops(true);

    QObject::connect(&settings::Settings::getHideTabX(),
                     SIGNAL(settings::BoolSetting::valueChanged(bool)), this,
                     SLOT(NotebookTab::hideTabXChanged(bool)));

    this->installEventFilter(this);

    this->setMouseTracking(true);
}

NotebookTab::~NotebookTab()
{
    QObject::disconnect(&settings::Settings::getHideTabX(),
                        SIGNAL(settings::BoolSetting::valueChanged(bool)), this,
                        SLOT(NotebookTab::hideTabXChanged(bool)));
}

void
NotebookTab::calcSize()
{
    if (settings::Settings::getHideTabX().get()) {
        this->resize(this->fontMetrics().width(this->text) + 8, 24);
    } else {
        this->resize(this->fontMetrics().width(this->text) + 8 + 24, 24);
    }
}

void
NotebookTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor fg = QColor(0, 0, 0);

    auto colorScheme = ColorScheme::instance();

    if (this->selected) {
        painter.fillRect(rect(), colorScheme.TabSelectedBackground);
        fg = colorScheme.TabSelectedText;
    } else if (this->mouseOver) {
        painter.fillRect(rect(), colorScheme.TabHoverBackground);
        fg = colorScheme.TabHoverText;
    } else if (this->highlightStyle == HighlightHighlighted) {
        painter.fillRect(rect(), colorScheme.TabHighlightedBackground);
        fg = colorScheme.TabHighlightedText;
    } else if (this->highlightStyle == HighlightNewMessage) {
        painter.fillRect(rect(), colorScheme.TabNewMessageBackground);
        fg = colorScheme.TabHighlightedText;
    } else {
        painter.fillRect(rect(), colorScheme.TabBackground);
        fg = colorScheme.TabText;
    }

    painter.setPen(fg);

    QRect rect(0, 0,
               width() - (settings::Settings::getHideTabX().get() ? 0 : 16),
               height());

    painter.drawText(rect, this->text, QTextOption(Qt::AlignCenter));

    if (!settings::Settings::getHideTabX().get() && (mouseOver || selected)) {
        if (mouseOverX) {
            painter.fillRect(getXRect(), QColor(0, 0, 0, 64));

            if (mouseDownX) {
                painter.fillRect(getXRect(), QColor(0, 0, 0, 64));
            }
        }

        painter.drawLine(getXRect().topLeft() + QPoint(4, 4),
                         getXRect().bottomRight() + QPoint(-4, -4));
        painter.drawLine(getXRect().topRight() + QPoint(-4, 4),
                         getXRect().bottomLeft() + QPoint(4, -4));
    }
}

void
NotebookTab::mousePressEvent(QMouseEvent *event)
{
    this->mouseDown = true;
    this->mouseDownX = this->getXRect().contains(event->pos());

    this->repaint();

    this->notebook->select(page);
}

void
NotebookTab::mouseReleaseEvent(QMouseEvent *event)
{
    this->mouseDown = false;

    if (this->mouseDownX) {
        this->mouseDownX = false;

        this->notebook->removePage(this->page);
    } else {
        repaint();
    }
}

void
NotebookTab::enterEvent(QEvent *)
{
    this->mouseOver = true;

    repaint();
}

void
NotebookTab::leaveEvent(QEvent *)
{
    this->mouseOverX = this->mouseOver = false;

    repaint();
}

void
NotebookTab::dragEnterEvent(QDragEnterEvent *event)
{
    this->notebook->select(page);
}

void
NotebookTab::mouseMoveEvent(QMouseEvent *event)
{
    bool overX = this->getXRect().contains(event->pos());

    if (overX != this->mouseOverX) {
        this->mouseOverX = overX;

        this->repaint();
    }
}
}
}
