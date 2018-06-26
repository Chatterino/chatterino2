#include "widgets/helper/SettingsDialogTab.hpp"
#include "widgets/SettingsDialog.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QPainter>
#include <QStyleOption>

namespace chatterino {
namespace widgets {

SettingsDialogTab::SettingsDialogTab(SettingsDialog *_dialog, settingspages::SettingsPage *_page,
                                     QString imageFileName)
    : BaseWidget(_dialog)
    , dialog(_dialog)
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

    int a = (this->height() - (20 * this->getScale())) / 2;
    QPixmap pixmap = this->ui.icon.pixmap(QSize(this->height() - a * 2, this->height() - a * 2));

    painter.drawPixmap(a, a, pixmap);

    a = a + a + 20 + a;

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
