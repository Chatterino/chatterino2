#include "widgets/helper/SettingsDialogTab.hpp"

#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QPainter>
#include <QStyleOption>

namespace chatterino {

SettingsDialogTab::SettingsDialogTab(SettingsDialog *_dialog,
                                     std::function<SettingsPage *()> _lazyPage,
                                     const QString &name, QString imageFileName,
                                     SettingsTabId id)
    : BaseWidget(_dialog)
    , dialog_(_dialog)
    , lazyPage_(std::move(_lazyPage))
    , id_(id)
    , name_(name)
{
    this->ui_.labelText = name;
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
    selectedChanged(selected_);
}

SettingsPage *SettingsDialogTab::page()
{
    if (this->page_)
    {
        return this->page_;
    }

    this->page_ = this->lazyPage_();
    this->page_->setTab(this);
    return this->page_;
}

void SettingsDialogTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QStyleOption opt;
    opt.initFrom(this);

    this->style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    int iconSize = 20 * this->scale();
    int pad = (this->height() - iconSize) / 2;
    QPixmap pixmap = this->ui_.icon.pixmap(
        QSize(this->height() - pad * 2, this->height() - pad * 2));

    painter.drawPixmap(pad, pad, pixmap);

    pad = (3 * pad) + iconSize;

    this->style()->drawItemText(&painter,
                                QRect(pad, 0, width() - pad, height()),
                                Qt::AlignLeft | Qt::AlignVCenter,
                                this->palette(), false, this->ui_.labelText);
}

void SettingsDialogTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    this->dialog_->selectTab(this);

    this->setFocus();
}

const QString &SettingsDialogTab::name() const
{
    return name_;
}

SettingsTabId SettingsDialogTab::id() const
{
    return id_;
}

}  // namespace chatterino
