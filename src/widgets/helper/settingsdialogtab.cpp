#include "widgets/helper/settingsdialogtab.hpp"
#include "widgets/settingsdialog.hpp"
#include "widgets/settingspages/settingspage.hpp"

#include <QPainter>
#include <QStyleOption>

namespace chatterino {
namespace widgets {

SettingsDialogTab::SettingsDialogTab(SettingsDialog *_dialog, settingspages::SettingsPage *_page,
                                     QString imageFileName)
    : dialog(_dialog)
    , page(_page)
{
    this->ui.labelText = page->getName();
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

settingspages::SettingsPage *SettingsDialogTab::getSettingsPage()
{
    return this->page;
}

void SettingsDialogTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QStyleOption opt;
    opt.init(this);

    this->style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    int a = (this->height() - 20) / 2;
    QPixmap pixmap = this->ui.icon.pixmap(QSize(20, 20));

    painter.drawPixmap(a, a, pixmap);

    a = a + a + 20;

    painter.drawText(QRect(a + a, 0, width() - a, height()), this->ui.labelText,
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
