#pragma once

#include "messages/Image.hpp"

#include <QSize>
#include <vector>

namespace chatterino {

class ImagePriorityOrder
{
public:
    ImagePriorityOrder(std::vector<ImagePtr> &&order);
    ImagePriorityOrder(ImagePriorityOrder &&other);

    ImagePriorityOrder(const ImagePriorityOrder &other) = delete;
    ImagePriorityOrder &operator=(const ImagePriorityOrder &other) = delete;
    ImagePriorityOrder &operator=(ImagePriorityOrder &&other) = delete;

    const ImagePtr &firstLoadedImage() const;
    const ImagePtr &getLoadedAndQueue() const;
    QSize firstLoadedImageSize() const;
    bool notAnimated() const;

private:
    enum AnimationFlag { Unknown, No, Yes };

    std::vector<ImagePtr> order_;
    mutable AnimationFlag animated_ = AnimationFlag::Unknown;
};

}  // namespace chatterino