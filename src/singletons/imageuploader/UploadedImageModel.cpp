#include "singletons/imageuploader/UploadedImageModel.hpp"

#include "common/SignalVectorModel.hpp"
#include "messages/Image.hpp"
#include "singletons/imageuploader/ImageUploader.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/listview/ImagePtrItemDelegate.hpp"

#include <qdatetime.h>
#include <qlocale.h>
#include <qnamespace.h>
#include <QObject>
#include <Qt>

namespace chatterino {
UploadedImageModel::UploadedImageModel(QObject *parent)
    : SignalVectorModel<UploadedImage>(4, parent)
{
}
// image, path, delete url

UploadedImage UploadedImageModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const UploadedImage &original)
{
    return original;
}

void UploadedImageModel::getRowFromItem(const UploadedImage &item,
                                        std::vector<QStandardItem *> &row)
{
    row[0]->setData(item.imageLink, ImagePtrItemDelegate::IMAGE_URL_ROLE);
    row[0]->setData(item.imageLink, UploadedImageModel::DOUBLE_CLICK_LINK_ROLE);
    row[0]->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    auto locale = QLocale::system();
    setStringItem(row[1],
                  locale.toString(QDateTime::fromSecsSinceEpoch(item.timestamp),
                                  QLocale::ShortFormat),
                  false);

    if (item.deletionLink.isNull())
    {
        setStringItem(row[2], "N/A", false);
    }
    else
    {
        setStringItem(row[2], "[Double click to open]", false);
        row[2]->setData(item.deletionLink, DOUBLE_CLICK_LINK_ROLE);
    }

    if (item.localPath.isNull())
    {
        setStringItem(row[3], "n/a", false);
    }
    else
    {
        setStringItem(row[3], item.localPath, true);
    }
}

}  // namespace chatterino
