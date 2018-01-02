#include "widgets/helper/settingsdialogtab.hpp"
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
    this->ui.icon.addFile(imageFileName);

    this->setCursor(QCursor(Qt::PointingHandCursor));

    this->setStyleSheet("color: #FFF");
}

void SettingsDialogTab::setSelected(bool _selected)
{
    if (this->selected == _selected) {
        return;
    }

    //    height: <checkbox-size>px;

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

    int a = (this->height() - 20) / 2;
    QPixmap pixmap = this->ui.icon.pixmap(QSize(20,20));


    painter.drawPixmap(0, a, pixmap);

    a = a + a + 20;

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
