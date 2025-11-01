#include "widgets/dialogs/font/PreviewWidget.hpp"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStringListModel>
#include <QToolButton>
#include <QVBoxLayout>

namespace chatterino {

PreviewWidget::PreviewWidget(const QFont &startFont, QWidget *parent)
    : QWidget(parent)
    , font(startFont)
{
    this->setMinimumHeight(60);
}

void PreviewWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter{this};

    painter.fillRect(this->rect(), this->palette().base());
    painter.setFont(this->font);
    painter.drawText(
        this->rect().adjusted(3, 3, -3, -3),
        Qt::AlignCenter | Qt::TextSingleLine,
        QStringLiteral("The quick brown fox jumps over the lazy dog"));
}

void PreviewWidget::setFont(const QFont &font)
{
    this->font = font;
    this->update();
}

}  // namespace chatterino
