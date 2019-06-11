#pragma once

#include "messages/Image.hpp"

namespace chatterino {
    class TooltipPreviewImage {
    public:
        static TooltipPreviewImage &getInstance();
        void setImage(ImagePtr image);

    private:
        TooltipPreviewImage();

    private:
        ImagePtr image_ = nullptr;
        std::vector<pajlada::Signals::ScopedConnection> connections_;
    };
}  // namespace chatterino
