#include "widgets/TooltipWidget.hpp"

#include "Application.hpp"
#include "messages/Image.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/WindowManager.hpp"

#include <QPainter>

namespace chatterino {

namespace {

    // https://stackoverflow.com/a/22646928
    void clearWidgetsFromLayout(QLayout *layout)
    {
        if (layout == nullptr)
        {
            return;
        }

        while (auto item = layout->takeAt(0))
        {
            delete item->widget();
            clearWidgetsFromLayout(item->layout());
        }
    }

}  // namespace

TooltipWidget *TooltipWidget::instance()
{
    static TooltipWidget *tooltipWidget = new TooltipWidget();
    return tooltipWidget;
}

TooltipWidget::TooltipWidget(BaseWidget *parent)
    : BaseWindow({BaseWindow::TopMost, BaseWindow::DontFocus,
                  BaseWindow::DisableLayoutSave},
                 parent)
{
    this->setStyleSheet("color: #fff; background: rgba(11, 11, 11, 0.8)");
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setWindowFlag(Qt::WindowStaysOnTopHint, true);

    this->setStayInScreenRect(true);

    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setContentsMargins(10, 5, 10, 5);
    this->setLayout(layout);
    this->layout_ = layout;

    this->connections_.managedConnect(getFonts()->fontChanged, [this] {
        this->updateFont();
    });
    this->updateFont();

    auto windows = getApp()->windows;
    this->connections_.managedConnect(windows->gifRepaintRequested, [this] {
        for (int i = 0; i < this->visibleEntries_; ++i)
        {
            auto entry = this->entryAt(i);
            if (entry && entry->animated())
            {
                entry->refreshPixmap();
            }
        }
    });

    this->connections_.managedConnect(windows->miscUpdate, [this] {
        for (int i = 0; i < this->visibleEntries_; ++i)
        {
            auto entry = this->entryAt(i);
            if (entry->getImage())
            {
                if (entry->refreshPixmap())
                {
                    this->attemptRefresh = false;
                    this->adjustSize();
                }
            }
        }
    });
}

void TooltipWidget::setRecord(const TooltipEntryRecord &record)
{
    this->setRecords({record});
}

void TooltipWidget::setRecords(const std::vector<TooltipEntryRecord> &records)
{
    if (records.size() > this->layout_->count())
    {
        // Need to add more TooltipEntry instances
        int requiredAmount = records.size() - this->layout_->count();
        for (int i = 0; i < requiredAmount; ++i)
        {
            this->layout_->addWidget(new TooltipEntry());
        }
    }

    this->setVisibleEntries(records.size());

    for (int i = 0; i < records.size(); ++i)
    {
        auto entry = this->entryAt(i);
        if (entry)
        {
            auto &record = records[i];
            entry->setImage(record.image);
            entry->setText(record.text);
            entry->setImageScale(record.customWidth, record.customHeight);
        }
    }
}

void TooltipWidget::setVisibleEntries(int n)
{
    for (int i = 0; i < this->layout_->count(); ++i)
    {
        if (auto entry = this->entryAt(i))
        {
            if (i <= n - 1)
            {
                entry->show();
            }
            else
            {
                entry->hide();
                entry->clearImage();
            }
        }
    }
    this->visibleEntries_ = n;
}

// May be nullptr
TooltipEntry *TooltipWidget::entryAt(int n)
{
    return dynamic_cast<TooltipEntry *>(this->layout_->itemAt(n)->widget());
}

void TooltipWidget::themeChangedEvent()
{
    //    this->setStyleSheet("color: #fff; background: #000");
}

void TooltipWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), QColor(0, 0, 0, int(0.8 * 255)));
}

void TooltipWidget::scaleChangedEvent(float)
{
    this->updateFont();
}

void TooltipWidget::updateFont()
{
    this->setFont(
        getFonts()->getFont(FontStyle::ChatMediumSmall, this->scale()));
}

void TooltipWidget::setWordWrap(bool wrap)
{
    for (int i = 0; i < this->visibleEntries_; ++i)
    {
        auto entry = this->entryAt(i);
        if (entry)
        {
            entry->setWordWrap(wrap);
        }
    }
}

void TooltipWidget::clearImage()
{
    this->setVisibleEntries(0);
}

void TooltipWidget::hideEvent(QHideEvent *)
{
    this->clearImage();
}

void TooltipWidget::showEvent(QShowEvent *)
{
    this->adjustSize();
}

void TooltipWidget::changeEvent(QEvent *)
{
    // clear parents event
}

void TooltipWidget::leaveEvent(QEvent *)
{
    // clear parents event
}

}  // namespace chatterino
