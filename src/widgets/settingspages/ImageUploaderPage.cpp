#include "widgets/settingspages/ImageUploaderPage.hpp"

#include "Application.hpp"
#include "singletons/imageuploader/ImageUploader.hpp"
#include "singletons/imageuploader/UploadedImageModel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"
#include "widgets/listview/ImagePtrItemDelegate.hpp"

#include <QBoxLayout>
#include <QDesktopServices>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QUrl>

namespace chatterino {

ImageUploaderPage::ImageUploaderPage()
    : imgDelegate_(new ImagePtrItemDelegate())
{
    LayoutCreator<ImageUploaderPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>();

    auto *model = getApp()->imageUploader->createModel(nullptr);
    //EditableModelView *view =
    //    layout.emplace<EditableModelView>(model).getElement();

    auto container = layout.emplace<QVBoxLayout>();
    container->setContentsMargins(0, 0, 0, 0);

    auto buttons = container.emplace<QHBoxLayout>();
    auto *forget = buttons.emplace<QPushButton>().getElement();
    forget->setText("Remove image from log");
    buttons->addStretch();

    auto *view = layout.emplace<QTableView>().getElement();
    view->setModel(model);

    view->setItemDelegateForColumn(0, this->imgDelegate_);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);

    view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    model->setHeaderData(0, Qt::Horizontal, "Image (double click to open)",
                         Qt::DisplayRole);

    view->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    model->setHeaderData(1, Qt::Horizontal, "Date uploaded", Qt::DisplayRole);

    view->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    model->setHeaderData(2, Qt::Horizontal, "Delete link", Qt::DisplayRole);

    view->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    model->setHeaderData(3, Qt::Horizontal, "Path", Qt::DisplayRole);

    QObject::connect(forget, &QPushButton::pressed, this, [view, model]() {
        auto selected = view->selectionModel()->selectedRows(0);
        std::vector<int> rows;
        for (auto &&index : selected)
        {
            rows.push_back(index.row());
        }

        std::sort(rows.begin(), rows.end(), std::greater{});

        for (auto &&row : rows)
        {
            model->removeRow(row);
        }
        getApp()->imageUploader->save();
    });
    QObject::connect(
        view, &QTableView::doubleClicked, [](const QModelIndex &clicked) {
            auto url = clicked.data(UploadedImageModel::DOUBLE_CLICK_LINK_ROLE)
                           .toString();
            QDesktopServices::openUrl(QUrl(url));
        });
}

ImageUploaderPage::~ImageUploaderPage()
{
    delete this->imgDelegate_;
}

}  // namespace chatterino
