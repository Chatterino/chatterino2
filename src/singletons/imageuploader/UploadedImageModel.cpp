#include "singletons/imageuploader/UploadedImageModel.hpp"

#include "common/SignalVectorModel.hpp"
#include "messages/Image.hpp"
#include "singletons/imageuploader/ImageUploader.hpp"
#include "singletons/Resources.hpp"
#include "util/LoadPixmap.hpp"
#include "util/PostToThread.hpp"
#include "util/StandardItemHelper.hpp"

#include <qdatetime.h>
#include <qlocale.h>
#include <qnamespace.h>
#include <QObject>
#include <Qt>

namespace chatterino {
UploadedImageModel::UploadedImageModel(QObject *parent)
    : SignalVectorModel<UploadedImage>(1, parent)
{
}

UploadedImage UploadedImageModel::getItemFromRow(
    std::vector<QStandardItem *> & /* row */, const UploadedImage &original)
{
    return original;
}

void UploadedImageModel::getRowFromItem(const UploadedImage &item,
                                        std::vector<QStandardItem *> &row)
{
    row[0]->setData(item.imageLink, Qt::DisplayRole);
    row[0]->setData(item.imageLink, UploadedImageModel::DOUBLE_CLICK_LINK_ROLE);
    row[0]->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    loadPixmapFromUrl({item.imageLink},
                      [row](const QPixmap &pixmap) {
                          postToThread([pixmap, row]() {
                              row[0]->setData(pixmap, Qt::DecorationRole);
                          });
                      },
                      {[row]() {
                          postToThread([row]() {
                              row[0]->setData(getResources().imageFailedToLoad,
                                              Qt::DecorationRole);
                          });
                      }});
}

}  // namespace chatterino
