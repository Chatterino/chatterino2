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
    connections_.push_back(getApp()->windows->gifRepaintRequested.connect([&] {
        auto tooltipWidget = TooltipWidget::instance();
        if (this->image_ && !tooltipWidget->isHidden())
        {
            auto pixmap = this->image_->pixmapOrLoad();
            if (pixmap)
            {
                tooltipWidget->setImage(*pixmap);
            }
        }
        else
        {
            tooltipWidget->clearImage();
        }
    }));
}

void TooltipPreviewImage::setImage(ImagePtr image)
{
    this->image_ = image;
}
}  // namespace chatterino
