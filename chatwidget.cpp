#include "chatwidget.h"
#include "QFont"
#include "QFontDatabase"
#include "QPainter"
#include "QVBoxLayout"
#include "colorscheme.h"

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , m_vbox(this)
{
    m_vbox.setSpacing(0);
    m_vbox.setMargin(1);

    m_vbox.addWidget(&m_header);
    m_vbox.addWidget(&m_view);
    m_vbox.addWidget(&m_input);
}

ChatWidget::~ChatWidget()
{
}

void
ChatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::instance().ChatBackground);
}
