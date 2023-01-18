#include "TooltipEntry.hpp"

#include <QVBoxLayout>

namespace chatterino {

TooltipEntry::TooltipEntry(QWidget *parent)
    : TooltipEntry(nullptr, "", 0, 0, parent)
{
}

TooltipEntry::TooltipEntry(ImagePtr image, const QString &text, QWidget *parent)
    : TooltipEntry(image, text, 0, 0, parent)
{
}

TooltipEntry::TooltipEntry(ImagePtr image, const QString &text, int customWidth,
                           int customHeight, QWidget *parent)
    : QWidget(parent)
    , image_(image)
    , customImgWidth_(customWidth)
    , customImgHeight_(customHeight)
{
    auto layout = new QVBoxLayout(this);
    this->setLayout(layout);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setContentsMargins(10, 5, 10, 5);

    this->displayImage_ = new QLabel();
    this->displayImage_->setAlignment(Qt::AlignHCenter);
    this->displayImage_->setStyleSheet("background: transparent");
    this->displayText_ = new QLabel(text);
    this->displayText_->setAlignment(Qt::AlignHCenter);
    this->displayText_->setStyleSheet("background: transparent");

    layout->addWidget(this->displayImage_, Qt::AlignHCenter);
    layout->addWidget(this->displayText_, Qt::AlignHCenter);
}

void TooltipEntry::setWordWrap(bool wrap)
{
    this->displayText_->setWordWrap(wrap);
}

void TooltipEntry::setImageScale(int w, int h)
{
    if (this->customImgWidth_ == w && this->customImgHeight_ == h)
    {
        return;
    }
    this->customImgWidth_ = w;
    this->customImgHeight_ = h;
    this->refreshPixmap();
}

void TooltipEntry::setText(const QString &text)
{
    this->displayText_->setText(text);
}

void TooltipEntry::setImage(ImagePtr image)
{
    if (this->image_ == image)
    {
        return;
    }

    this->clearImage();
    this->image_ = std::move(image);
    this->refreshPixmap();
}

void TooltipEntry::clearImage()
{
    this->displayImage_->hide();
    this->image_ = nullptr;
    this->setImageScale(0, 0);
}

bool TooltipEntry::refreshPixmap()
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

    if (this->customImgWidth_ > 0 || this->customImgHeight_ > 0)
    {
        this->displayImage_->setPixmap(pixmap->scaled(this->customImgWidth_,
                                                      this->customImgHeight_,
                                                      Qt::KeepAspectRatio));
    }
    else
    {
        this->displayImage_->setPixmap(*pixmap);
    }
    this->displayImage_->show();

    return true;
}

bool TooltipEntry::animated() const
{
    return this->image_ && this->image_->animated();
}

ImagePtr TooltipEntry::getImage() const
{
    return this->image_;
}

}  // namespace chatterino
