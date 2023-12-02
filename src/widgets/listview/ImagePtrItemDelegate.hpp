#pragma once

#include "common/QLogging.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"

#include <QModelIndex>
#include <QPainter>
#include <QStyledItemDelegate>
#include <Qt>
#include <QTableView>

namespace chatterino {
class ImagePtrItemDelegate : public QStyledItemDelegate
{
    std::map<QString, ImagePtr> ownedImages_;
    QTableView *view_;

public:
    ImagePtrItemDelegate(QTableView *view)
        : view_(view)
    {
    }
    static constexpr auto IMAGE_URL_ROLE = Qt::UserRole + 1;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        auto url = index.data(IMAGE_URL_ROLE).toString();
        if (auto it = this->ownedImages_.find(url);
            it != this->ownedImages_.end())
        {
            auto img = it->second;
            auto opt = img->pixmapOrLoad();
            if (!opt)  // wait for next time
            {
                if (img->isEmpty())
                {
                    painter->drawText(option.rect, "[Error]");
                }
                else
                {
                    painter->drawText(option.rect, "Loading");
                }
                return;
            }
            auto pixmap = *opt;
            double aspectRatio =
                double(pixmap.width()) / double(pixmap.height());
            int width = int(option.rect.height() * aspectRatio);
            auto outputRect = QRect(option.rect.topLeft(),
                                    QSize(width, option.rect.height()));
            if (option.rect.width() < width)
            {
                // too wide?

                double revAspectRatio =
                    double(pixmap.height()) / double(pixmap.width());
                int height = int(option.rect.width() * revAspectRatio);

                outputRect = QRect(option.rect.topLeft(),
                                   QSize(option.rect.width(), height));
                if (option.rect.height() < height)
                {
                    // give up and squish the image
                    outputRect = option.rect;
                }
            }

            painter->drawPixmap(outputRect, pixmap, pixmap.rect());
        }
        auto img = Image::fromUrl(Url{url});

        img->pixmapOrLoad([this, index]() {
            // wait for it to parse
            QTimer::singleShot(100, [this, index]() {
                this->view_->repaint();
                this->view_->update(index);
            });
        });

        // You cannot stop me, clang-tidy
        auto *bleh = const_cast<ImagePtrItemDelegate *>(this);
        bleh->ownedImages_[url] = img;
    }
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        return {32, 32};
    }
};
}  // namespace chatterino
