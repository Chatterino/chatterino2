#include "widgets/helper/SettingsDialogTab.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QPainter>
#include <QStyleOption>

namespace chatterino {

SettingsDialogTab::SettingsDialogTab(SettingsDialog *_dialog,
                                     SettingsPage *_page, QString imageFileName)
    : BaseWidget(_dialog)
    , dialog_(_dialog)
    , page_(_page)
{
    this->ui_.labelText = page_->getName();
    this->ui_.icon.addFile(imageFileName);

    this->setCursor(QCursor(Qt::PointingHandCursor));

    this->setStyleSheet("color: #FFF");
}

void SettingsDialogTab::setSelected(bool _selected)
{
    if (this->selected_ == _selected)
    {
        return;
    }

    //    height: <checkbox-size>px;

    this->selected_ = _selected;
    emit selectedChanged(selected_);
}

SettingsPage *SettingsDialogTab::getSettingsPage()
{
    return this->page_;
}

void SettingsDialogTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QStyleOption opt;
    opt.init(this);

    this->style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    int a = (this->height() - (20 * this->scale())) / 2;
    QPixmap pixmap = this->ui_.icon.pixmap(
        QSize(this->height() - a * 2, this->height() - a * 2));

    painter.drawPixmap(a, a, pixmap);

    a = a + a + 20 + a;

    painter.drawText(QRect(a, 0, width() - a, height()), this->ui_.labelText,
                     QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

void SettingsDialogTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    this->dialog_->selectTab(this);
}

}  // namespace chatterino
