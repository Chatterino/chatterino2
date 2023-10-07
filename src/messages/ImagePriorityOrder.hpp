#pragma once

#include "messages/Image.hpp"

#include <QSize>

#include <vector>

namespace chatterino {

class ImagePriorityOrder final
{
public:
    ImagePriorityOrder() = delete;
    ~ImagePriorityOrder() = default;

    explicit ImagePriorityOrder(std::vector<ImagePtr> &&order);

    ImagePriorityOrder(ImagePriorityOrder &&other);
    ImagePriorityOrder &operator=(ImagePriorityOrder &&other) = delete;

    ImagePriorityOrder(const ImagePriorityOrder &other) = delete;
    ImagePriorityOrder &operator=(const ImagePriorityOrder &other) = delete;

    const ImagePtr &firstLoadedImage() const;
    const ImagePtr &getLoadedAndQueue() const;
    QSize firstLoadedImageSize() const;

    /**
     * Returns true if the image is known to be static (i.e. not animated)
     * If the image has not been loaded, meaning we don't know if it's static or not, this will return false
     */
    bool isStatic() const;

private:
    /**
     * Attempt to figure out whether this image is animated or not, and store its state in `animated_`
     */
    void loadAnimatedFlag() const;

    enum AnimationFlag {
        Unknown,
        No,
        Yes,
    };

    std::vector<ImagePtr> order_;
    mutable AnimationFlag animated_ = AnimationFlag::Unknown;
};

}  // namespace chatterino
