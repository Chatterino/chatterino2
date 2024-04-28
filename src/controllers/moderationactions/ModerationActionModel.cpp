#include "controllers/moderationactions/ModerationActionModel.hpp"

#include "Application.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "messages/Image.hpp"
#include "util/StandardItemHelper.hpp"

#include <QIcon>
#include <QLoggingCategory>
#include <QPixmap>
namespace {

using namespace chatterino;

// This was copied from TwitchBadges::loadEmoteImage and modified.
void loadEmoteImage(const ImagePtr &image,
                    std::function<void(QPixmap)> &&callback)
{
    auto url = image->url().string;
    NetworkRequest(url)
        .concurrent()
        .cache()
        .onSuccess(
            [callback = std::move(callback), url](const NetworkResult &result) {
                auto data = result.getData();

                // const cast since we are only reading from it
                QBuffer buffer(const_cast<QByteArray *>(&data));
                buffer.open(QIODevice::ReadOnly);
                QImageReader reader(&buffer);

                if (!reader.canRead() || reader.size().isEmpty())
                {
                    qCWarning(chatterinoSettings)
                        << "Can't read mod action image at" << url << ":"
                        << reader.errorString();
                    return;
                }

                QImage image = reader.read();
                if (image.isNull())
                {
                    qCWarning(chatterinoSettings)
                        << "Failed reading mod action image at" << url << ":"
                        << reader.errorString();
                    return;
                }

                callback(QPixmap::fromImage(image));
            })
        .execute();
}
}  // namespace

namespace chatterino {

// commandmodel
ModerationActionModel ::ModerationActionModel(QObject *parent)
    : SignalVectorModel<ModerationAction>(2, parent)
{
}

// turn a vector item into a model row
ModerationAction ModerationActionModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const ModerationAction &original)
{
    return ModerationAction(
        row[Column::Command]->data(Qt::DisplayRole).toString(),
        row[Column::Icon]->data(Qt::UserRole).toString());
}

// turns a row in the model into a vector item
void ModerationActionModel::getRowFromItem(const ModerationAction &item,
                                           std::vector<QStandardItem *> &row)
{
    setStringItem(row[Column::Command], item.getAction());
    setFilePathItem(row[Column::Icon], item.iconPath());
    if (!item.iconPath().isEmpty())
    {
        loadEmoteImage(*item.getImage(), [row](const QPixmap &pixmap) {
            row[Column::Icon]->setData(pixmap, Qt::DecorationRole);
        });
    }
}

}  // namespace chatterino
