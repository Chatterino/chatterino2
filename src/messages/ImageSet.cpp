#include "messages/ImageSet.hpp"

#include "messages/Image.hpp"
#include "messages/ImagePriorityOrder.hpp"
#include "singletons/Settings.hpp"

namespace {

using namespace chatterino;

bool isValidImagePtr(const ImagePtr &img)
{
    return img && !img->isEmpty();
}

}  // namespace

namespace chatterino {

ImageSet::ImageSet()
    : imageX1_(Image::getEmpty())
    , imageX2_(Image::getEmpty())
    , imageX3_(Image::getEmpty())
{
}

ImageSet::ImageSet(const ImagePtr &image1, const ImagePtr &image2,
                   const ImagePtr &image3)
    : imageX1_(image1)
    , imageX2_(image2)
    , imageX3_(image3)
{
}

ImageSet::ImageSet(const Url &image1, const Url &image2, const Url &image3)
    : imageX1_(Image::fromUrl(image1, 1))
    , imageX2_(image2.string.isEmpty() ? Image::getEmpty()
                                       : Image::fromUrl(image2, 0.5))
    , imageX3_(image3.string.isEmpty() ? Image::getEmpty()
                                       : Image::fromUrl(image3, 0.25))
{
}

void ImageSet::setImage1(const ImagePtr &image)
{
    this->imageX1_ = image;
}

void ImageSet::setImage2(const ImagePtr &image)
{
    this->imageX2_ = image;
}

void ImageSet::setImage3(const ImagePtr &image)
{
    this->imageX3_ = image;
}

const ImagePtr &ImageSet::getImage1() const
{
    return this->imageX1_;
}

const ImagePtr &ImageSet::getImage2() const
{
    return this->imageX2_;
}

const ImagePtr &ImageSet::getImage3() const
{
    return this->imageX3_;
}

const std::shared_ptr<Image> &getImagePriv(const ImageSet &set, float scale)
{
    scale *= getSettings()->emoteScale;

    int quality = 1;

    if (scale > 2.001f)
        quality = 3;
    else if (scale > 1.001f)
        quality = 2;

    if (!set.getImage3()->isEmpty() && quality == 3)
    {
        return set.getImage3();
    }

    if (!set.getImage2()->isEmpty() && quality >= 2)
    {
        return set.getImage2();
    }

    return set.getImage1();
}

const ImagePtr &ImageSet::getImage(float scale) const
{
    return getImagePriv(*this, scale);
}

std::optional<ImagePriorityOrder> ImageSet::getPriority(float scale) const
{
    std::vector<ImagePtr> result;
    result.reserve(4);

    auto pushIfNotEmpty = [&result](const ImagePtr &img) {
        if (isValidImagePtr(img))
        {
            result.push_back(img);
        }
    };

    pushIfNotEmpty(this->getImage(scale));
    pushIfNotEmpty(this->imageX3_);
    pushIfNotEmpty(this->imageX2_);
    pushIfNotEmpty(this->imageX1_);

    if (result.empty())
    {
        return std::nullopt;
    }

    return ImagePriorityOrder(std::move(result));
}

bool ImageSet::operator==(const ImageSet &other) const
{
    return std::tie(this->imageX1_, this->imageX2_, this->imageX3_) ==
           std::tie(other.imageX1_, other.imageX2_, other.imageX3_);
}

bool ImageSet::operator!=(const ImageSet &other) const
{
    return !this->operator==(other);
}

}  // namespace chatterino
