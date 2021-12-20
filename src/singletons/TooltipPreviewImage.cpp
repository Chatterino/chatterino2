#include "TooltipPreviewImage.hpp"

#include "Application.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/TooltipWidget.hpp"

namespace chatterino {

TooltipPreviewImage &TooltipPreviewImage::instance()
{
    static TooltipPreviewImage *instance = new TooltipPreviewImage();
    return *instance;
}

TooltipPreviewImage::TooltipPreviewImage()
{
    auto windows = getApp()->windows;

    this->connections_.managedConnect(windows->gifRepaintRequested, [&] {
        if (this->image_ && this->image_->animated())
        {
            this->refreshTooltipWidgetPixmap();
        }
    });

    this->connections_.managedConnect(windows->miscUpdate, [&] {
        if (this->attemptRefresh)
        {
            this->refreshTooltipWidgetPixmap();
        }
    });
}

void TooltipPreviewImage::setImage(ImagePtr image)
{
    this->image_ = std::move(image);

    this->refreshTooltipWidgetPixmap();
}

void TooltipPreviewImage::setImageScale(int w, int h)
{
    this->imageWidth_ = w;
    this->imageHeight_ = h;

    this->refreshTooltipWidgetPixmap();
}

void TooltipPreviewImage::refreshTooltipWidgetPixmap()
{
    auto tooltipWidget = TooltipWidget::instance();

    if (this->image_ && !tooltipWidget->isHidden())
    {
        if (auto pixmap = this->image_->pixmapOrLoad())
        {
            if (this->imageWidth_ != 0 && this->imageHeight_)
            {
                tooltipWidget->setImage(pixmap->scaled(this->imageWidth_,
                                                       this->imageHeight_,
                                                       Qt::KeepAspectRatio));
            }
            else
            {
                tooltipWidget->setImage(*pixmap);
            }

            this->attemptRefresh = false;
        }
        else
        {
            this->attemptRefresh = true;
        }
    }
    else
    {
        tooltipWidget->clearImage();
    }
}

}  // namespace chatterino
