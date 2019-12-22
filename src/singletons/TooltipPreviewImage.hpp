#pragma once

#include "messages/Image.hpp"

namespace chatterino {
class TooltipPreviewImage
{
public:
    static TooltipPreviewImage &instance();
    void setImage(ImagePtr image);

    TooltipPreviewImage(const TooltipPreviewImage &) = delete;

private:
    TooltipPreviewImage();

private:
    ImagePtr image_ = nullptr;
    std::vector<pajlada::Signals::ScopedConnection> connections_;
};
}  // namespace chatterino
