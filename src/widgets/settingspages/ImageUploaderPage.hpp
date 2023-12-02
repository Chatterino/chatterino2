#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class ImagePtrItemDelegate;

class ImageUploaderPage : public SettingsPage
{
    ImagePtrItemDelegate *imgDelegate_;

public:
    ImageUploaderPage();
    ~ImageUploaderPage() override;

    ImageUploaderPage(ImageUploaderPage &cpy) = delete;
    ImageUploaderPage(ImageUploaderPage &&move) = delete;
    ImageUploaderPage &operator=(const ImageUploaderPage &copyAssign) = delete;
    ImageUploaderPage &&operator=(const ImageUploaderPage &&moveAssign) =
        delete;
};

}  // namespace chatterino
