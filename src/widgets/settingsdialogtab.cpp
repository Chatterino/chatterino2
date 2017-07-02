#include "widgets/settingsdialogtab.hpp"
#include "widgets/settingsdialog.hpp"

#include <QPainter>
#include <QStyleOption>

namespace chatterino {
namespace widgets {

SettingsDialogTab::SettingsDialogTab(SettingsDialog *_dialog, QString _labelText,
                                     QString imageFileName)
    : dialog(_dialog)
{
    this->ui.labelText = _labelText;
    this->ui.image.load(imageFileName);

    // XXX: DPI (not sure if this is auto-adjusted with custom DPI)
    this->setFixedHeight(32);

    this->setCursor(QCursor(Qt::PointingHandCursor));

    this->setStyleSheet("color: #FFF");
}

void SettingsDialogTab::setSelected(bool _selected)
{
    if (this->selected == _selected) {
        return;
    }

    this->selected = _selected;
    emit selectedChanged(selected);
}

QWidget *SettingsDialogTab::getWidget()
{
    return this->ui.widget;
}

void SettingsDialogTab::setWidget(QWidget *widget)
{
    this->ui.widget = widget;
}

void SettingsDialogTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QStyleOption opt;
    opt.init(this);

    this->style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    int a = (this->height() - this->ui.image.width()) / 2;

    painter.drawImage(a, a, this->ui.image);

    a = a + a + this->ui.image.width();

    painter.drawText(QRect(a, 0, width() - a, height()), this->ui.labelText,
                     QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

void SettingsDialogTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    this->dialog->select(this);
}

}  // namespace widgets
}  // namespace chatterino
