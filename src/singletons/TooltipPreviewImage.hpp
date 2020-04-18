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

    // attemptRefresh is set to true in case we want to preview an image that has not loaded yet (if pixmapOrLoad fails)
    bool attemptRefresh{false};

    // Refresh the pixmap used in the Tooltip Widget
    // Called from setImage and from the "gif repaint" signal if the image is animated
    void refreshTooltipWidgetPixmap();
};

}  // namespace chatterino
