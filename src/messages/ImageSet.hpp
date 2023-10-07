#pragma once

#include "common/Aliases.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace chatterino {

class ImagePriorityOrder;
class Image;
using ImagePtr = std::shared_ptr<Image>;
ImagePtr getEmptyImagePtr();

class ImageSet
{
public:
    ImageSet();
    ImageSet(const ImagePtr &image1,
             const ImagePtr &image2 = getEmptyImagePtr(),
             const ImagePtr &image3 = getEmptyImagePtr());
    ImageSet(const Url &image1, const Url &image2 = {}, const Url &image3 = {});

    void setImage1(const ImagePtr &image);
    void setImage2(const ImagePtr &image);
    void setImage3(const ImagePtr &image);
    const ImagePtr &getImage1() const;
    const ImagePtr &getImage2() const;
    const ImagePtr &getImage3() const;

    const ImagePtr &getImage(float scale) const;
    std::optional<ImagePriorityOrder> getPriority(float scale) const;

    bool operator==(const ImageSet &other) const;
    bool operator!=(const ImageSet &other) const;

private:
    ImagePtr imageX1_;
    ImagePtr imageX2_;
    ImagePtr imageX3_;
};

}  // namespace chatterino
