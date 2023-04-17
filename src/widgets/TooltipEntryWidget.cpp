#include "widgets/TooltipEntryWidget.hpp"

#include <QVBoxLayout>

namespace chatterino {

TooltipEntryWidget::TooltipEntryWidget(QWidget *parent)
    : TooltipEntryWidget(nullptr, "", 0, 0, parent)
{
}

TooltipEntryWidget::TooltipEntryWidget(ImagePtr image, const QString &text,
                                       int customWidth, int customHeight,
                                       QWidget *parent)
    : QWidget(parent)
    , image_(image)
    , customImgWidth_(customWidth)
    , customImgHeight_(customHeight)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    this->displayImage_ = new QLabel();
    this->displayImage_->setAlignment(Qt::AlignHCenter);
    this->displayImage_->setStyleSheet("background: transparent");
    this->displayImage_->hide();
    this->displayText_ = new QLabel(text);
    this->displayText_->setAlignment(Qt::AlignHCenter);
    this->displayText_->setStyleSheet("background: transparent");

    layout->addWidget(this->displayImage_);
    layout->addWidget(this->displayText_);
}

void TooltipEntryWidget::setWordWrap(bool wrap)
{
    this->displayText_->setWordWrap(wrap);
}

void TooltipEntryWidget::setImageScale(int w, int h)
{
    if (this->customImgWidth_ == w && this->customImgHeight_ == h)
    {
        return;
    }
    this->customImgWidth_ = w;
    this->customImgHeight_ = h;
    this->refreshPixmap();
}

void TooltipEntryWidget::setText(const QString &text)
{
    this->displayText_->setText(text);
}

void TooltipEntryWidget::setImage(ImagePtr image)
{
    if (this->image_ == image)
    {
        return;
    }

    this->clearImage();
    this->image_ = std::move(image);
    this->refreshPixmap();
}

void TooltipEntryWidget::clearImage()
{
    this->displayImage_->hide();
    this->image_ = nullptr;
    this->setImageScale(0, 0);
}

bool TooltipEntryWidget::refreshPixmap()
{
    if (!this->image_)
    {
        return false;
    }

    auto pixmap = this->image_->pixmapOrLoad();
    if (!pixmap)
    {
        this->attemptRefresh_ = true;
        return false;
    }
    pixmap->setDevicePixelRatio(this->devicePixelRatio());

    if (this->customImgWidth_ > 0 || this->customImgHeight_ > 0)
    {
        this->displayImage_->setPixmap(pixmap->scaled(this->customImgWidth_,
                                                      this->customImgHeight_,
                                                      Qt::KeepAspectRatio));
        if (this->displayImage_->size() !=
            QSize{this->customImgWidth_, this->customImgHeight_})
        {
            this->adjustSize();
        }
    }
    else
    {
        this->displayImage_->setPixmap(*pixmap);
    }
    this->displayImage_->show();

    return true;
}

bool TooltipEntryWidget::animated() const
{
    return this->image_ && this->image_->animated();
}

bool TooltipEntryWidget::hasImage() const
{
    return this->image_ != nullptr;
}

bool TooltipEntryWidget::attemptRefresh() const
{
    return this->attemptRefresh_;
}

}  // namespace chatterino
