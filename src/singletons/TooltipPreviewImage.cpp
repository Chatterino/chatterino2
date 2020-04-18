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

    this->connections_.push_back(windows->gifRepaintRequested.connect([&] {
        if (this->image_ && this->image_->animated())
        {
            this->refreshTooltipWidgetPixmap();
        }
    }));

    this->connections_.push_back(windows->miscUpdate.connect([&] {
        if (this->attemptRefresh)
        {
            this->refreshTooltipWidgetPixmap();
        }
    }));
}

void TooltipPreviewImage::setImage(ImagePtr image)
{
    this->image_ = image;

    this->refreshTooltipWidgetPixmap();
}

void TooltipPreviewImage::refreshTooltipWidgetPixmap()
{
    auto tooltipWidget = TooltipWidget::instance();

    if (this->image_ && !tooltipWidget->isHidden())
    {
        if (auto pixmap = this->image_->pixmapOrLoad())
        {
            tooltipWidget->setImage(*pixmap);
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
