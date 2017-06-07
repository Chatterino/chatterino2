#include "widgets/settingsdialogtab.h"
#include "widgets/settingsdialog.h"

#include <QPainter>
#include <QStyleOption>

namespace chatterino {
namespace widgets {

SettingsDialogTab::SettingsDialogTab(SettingsDialog *dialog, QString label, QString imageRes)
    : _label(label)
    , _image(QImage(imageRes))
    , _dialog(dialog)
    , _selected(false)
{
    setFixedHeight(32);

    setCursor(QCursor(Qt::PointingHandCursor));

    setStyleSheet("color: #FFF");
}

void SettingsDialogTab::setSelected(bool selected)
{
    if (_selected == selected)
        return;

    _selected = selected;
    emit selectedChanged(selected);
}

bool SettingsDialogTab::getSelected() const
{
    return _selected;
}

QWidget *SettingsDialogTab::getWidget()
{
    return _widget;
}

void SettingsDialogTab::setWidget(QWidget *widget)
{
    _widget = widget;
}

void SettingsDialogTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QStyleOption opt;
    opt.init(this);

    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    int a = (height() - _image.width()) / 2;

    painter.drawImage(a, a, _image);

    a = a + a + _image.width();

    painter.drawText(QRect(a, 0, width() - a, height()), _label,
                     QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

void SettingsDialogTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    _dialog->select(this);
}

}  // namespace widgets
}  // namespace chatterino
