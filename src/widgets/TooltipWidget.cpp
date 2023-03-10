#include "widgets/TooltipWidget.hpp"

#include "Application.hpp"
#include "messages/Image.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/WindowManager.hpp"

#include <QPainter>
#include <QVBoxLayout>

namespace chatterino {

TooltipWidget *TooltipWidget::instance()
{
    static TooltipWidget *tooltipWidget = new TooltipWidget();
    return tooltipWidget;
}

TooltipWidget::TooltipWidget(BaseWidget *parent)
    : BaseWindow({BaseWindow::TopMost, BaseWindow::DontFocus,
                  BaseWindow::DisableLayoutSave},
                 parent)
    , displayImage_(new QLabel(this))
    , displayText_(new QLabel(this))
{
    this->setStyleSheet("color: #fff; background: rgba(11, 11, 11, 0.8)");
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setWindowFlag(Qt::WindowStaysOnTopHint, true);

    this->setStayInScreenRect(true);

    displayImage_->setAlignment(Qt::AlignHCenter);
    displayImage_->setStyleSheet("background: transparent");

    displayText_->setAlignment(Qt::AlignHCenter);
    displayText_->setStyleSheet("background: transparent");

    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->addWidget(displayImage_);
    layout->addWidget(displayText_);
    this->setLayout(layout);

    this->connections_.managedConnect(getFonts()->fontChanged, [this] {
        this->updateFont();
    });
    this->updateFont();

    auto windows = getApp()->windows;
    this->connections_.managedConnect(windows->gifRepaintRequested, [this] {
        if (this->image_ && this->image_->animated())
        {
            this->refreshPixmap();
        }
    });

    this->connections_.managedConnect(windows->miscUpdate, [this] {
        if (this->image_ && this->attemptRefresh)
        {
            if (this->refreshPixmap())
            {
                this->attemptRefresh = false;
                this->adjustSize();
            }
        }
    });
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

void TooltipWidget::setText(QString text)
{
    this->displayText_->setText(text);
}

void TooltipWidget::setWordWrap(bool wrap)
{
    this->displayText_->setWordWrap(wrap);
}

void TooltipWidget::clearImage()
{
    this->displayImage_->hide();
    this->image_ = nullptr;
    this->setImageScale(0, 0);
}

void TooltipWidget::setImage(ImagePtr image)
{
    if (this->image_ == image)
    {
        return;
    }
    // hide image until loaded and reset scale
    this->clearImage();
    this->image_ = std::move(image);
    this->refreshPixmap();
}

void TooltipWidget::setImageScale(int w, int h)
{
    if (this->customImgWidth == w && this->customImgHeight == h)
    {
        return;
    }
    this->customImgWidth = w;
    this->customImgHeight = h;
    this->refreshPixmap();
}

void TooltipWidget::hideEvent(QHideEvent *)
{
    this->clearImage();
}

void TooltipWidget::showEvent(QShowEvent *)
{
    this->adjustSize();
}

bool TooltipWidget::refreshPixmap()
{
    if (!this->image_)
    {
        return false;
    }

    auto pixmap = this->image_->pixmapOrLoad();
    if (!pixmap)
    {
        this->attemptRefresh = true;
        return false;
    }

    if (this->customImgWidth > 0 || this->customImgHeight > 0)
    {
        this->displayImage_->setPixmap(pixmap->scaled(
            this->customImgWidth, this->customImgHeight, Qt::KeepAspectRatio));
    }
    else
    {
        this->displayImage_->setPixmap(*pixmap);
    }
    this->displayImage_->show();

    return true;
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
