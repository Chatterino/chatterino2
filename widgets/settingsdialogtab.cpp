#include "widgets/settingsdialogtab.h"
#include "widgets/settingsdialog.h"

#include <QPainter>
#include <QStyleOption>

namespace chatterino {
namespace widgets {

SettingsDialogTab::SettingsDialogTab(SettingsDialog *dialog, QString label,
                                     QString imageRes)
    : image(QImage(imageRes))
{
    this->dialog = dialog;

    this->label = label;
    setFixedHeight(32);

    setCursor(QCursor(Qt::PointingHandCursor));
}

void
SettingsDialogTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QStyleOption opt;
    opt.init(this);

    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    int a = (height() - image.width()) / 2;

    painter.drawImage(a, a, image);

    a = a + a + image.width();

    painter.drawText(QRect(a, 0, width() - a, height()), label,
                     QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

void
SettingsDialogTab::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    dialog->select(this);
}
}
}
