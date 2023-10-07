#include "messages/ImagePriorityOrder.hpp"

#include "common/QLogging.hpp"
#include "messages/Image.hpp"

namespace chatterino {

ImagePriorityOrder::ImagePriorityOrder(std::vector<ImagePtr> &&order)
    : order_(std::move(order))
{
    assert(!this->order_.empty());
}

ImagePriorityOrder::ImagePriorityOrder(ImagePriorityOrder &&other)
    : order_(std::move(other.order_))
{
}

const ImagePtr &ImagePriorityOrder::firstLoadedImage() const
{
    for (const auto &img : this->order_)
    {
        if (img != nullptr && !img->isEmpty() && img->loaded())
        {
            return img;
        }
    }

    // fallback to first-priority image if nothing else is loaded
    return this->order_.front();
}

const ImagePtr &ImagePriorityOrder::getLoadedAndQueue() const
{
    const auto &firstLoaded = this->firstLoadedImage();
    if (firstLoaded != this->order_.front())
    {
        // The image we have already loaded isn't the desired image.
        // Queue up loading the desired image, but use the already loaded one
        // for painting this time around.
        this->order_.front()->loadIfUnloaded();
    }
    else if (!firstLoaded->loaded())
    {
        // firstLoaded is the first-priority image, but it isn't loaded.
        firstLoaded->loadIfUnloaded();
    }

    return firstLoaded;
}

QSize ImagePriorityOrder::firstLoadedImageSize() const
{
    return this->firstLoadedImage()->size();
}

bool ImagePriorityOrder::isStatic() const
{
    if (this->animated_ == AnimationFlag::Unknown)
    {
        this->loadAnimatedFlag();
    }

    if (this->animated_ != AnimationFlag::Unknown)
    {
        // We have already checked the loaded image at some point
        return this->animated_ == AnimationFlag::No;
    }

    // We aren't sure if the image is animated or not because it isn't loaded.
    return false;
}

void ImagePriorityOrder::loadAnimatedFlag() const
{
    // This function should only be called when the animation flag is unknown
    assert(this->animated_ == AnimationFlag::Unknown);

    const auto &firstLoaded = this->firstLoadedImage();
    if (firstLoaded->loaded())
    {
        if (firstLoaded->animated())
        {
            this->animated_ = AnimationFlag::Yes;
        }
        else
        {
            this->animated_ = AnimationFlag::No;
        }
    }
}

}  // namespace chatterino
