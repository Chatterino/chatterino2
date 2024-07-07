#pragma once

#include "common/SignalVectorModel.hpp"
#include "singletons/imageuploader/ImageUploader.hpp"

#include <QObject>
#include <Qt>

namespace chatterino {

struct UploadedImage;
class UploadedImageModel : public SignalVectorModel<UploadedImage>
{
    explicit UploadedImageModel(QObject *parent);
    friend class ImageUploader;

protected:
    UploadedImage getItemFromRow(std::vector<QStandardItem *> &row,
                                 const UploadedImage &original) override;

    void getRowFromItem(const UploadedImage &item,
                        std::vector<QStandardItem *> &row) override;

public:
    static constexpr auto DOUBLE_CLICK_LINK_ROLE = Qt::UserRole + 2;
};

}  // namespace chatterino
