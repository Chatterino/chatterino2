#include "widgets/settingspages/ImageUploaderPage.hpp"

#include "Application.hpp"
#include "singletons/imageuploader/ImageUploader.hpp"
#include "singletons/imageuploader/UploadedImageModel.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"
#include "widgets/listview/ImagePtrItemDelegate.hpp"

#include <QBoxLayout>
#include <QDesktopServices>
#include <QFormLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QTabWidget>
#include <QUrl>

namespace chatterino {

ImageUploaderPage::ImageUploaderPage()
{
    LayoutCreator<ImageUploaderPage> layoutCreator(this);
    auto tabs = layoutCreator.emplace<QTabWidget>();

    {
        auto layout = tabs.appendTab(new QVBoxLayout, "Logs");

        auto *model = getApp()->getImageUploader()->createModel(nullptr);

        auto container = layout.emplace<QVBoxLayout>();
        container->setContentsMargins(0, 0, 0, 0);

        auto buttons = container.emplace<QHBoxLayout>();
        buttons->setContentsMargins(0, 0, 0, 0);
        auto *forget = buttons.emplace<QPushButton>().getElement();
        forget->setText("Remove image from log");
        buttons->addStretch();

        auto *view = layout.emplace<QTableView>().getElement();
        view->setModel(model);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);

        view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        model->setHeaderData(0, Qt::Horizontal, "Image (double click to open)",
                             Qt::DisplayRole);

        view->horizontalHeader()->setSectionResizeMode(
            1, QHeaderView::ResizeToContents);
        model->setHeaderData(1, Qt::Horizontal, "Date uploaded",
                             Qt::DisplayRole);

        view->horizontalHeader()->setSectionResizeMode(
            2, QHeaderView::ResizeToContents);
        model->setHeaderData(2, Qt::Horizontal, "Delete link", Qt::DisplayRole);

        view->horizontalHeader()->setSectionResizeMode(
            3, QHeaderView::Interactive);
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
            getApp()->getImageUploader()->save();
        });
        QObject::connect(
            view, &QTableView::doubleClicked, [](const QModelIndex &clicked) {
                auto url =
                    clicked.data(UploadedImageModel::DOUBLE_CLICK_LINK_ROLE)
                        .toString();
                QDesktopServices::openUrl(QUrl(url));
            });
    }
    {
        auto groupLayout = tabs.appendTab(new QFormLayout, "Settings");

        auto *description = new QLabel(
            "You can set custom host for uploading images, like "
            "imgur.com or s-ul.eu.<br>Check " +
            formatRichNamedLink("https://chatterino.com/help/image-uploader",
                                "this guide") +
            " for help.");
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");
        description->setTextFormat(Qt::RichText);
        description->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                             Qt::LinksAccessibleByKeyboard |
                                             Qt::LinksAccessibleByMouse);
        description->setOpenExternalLinks(true);

        groupLayout->setWidget(0, QFormLayout::SpanningRole, description);

        groupLayout->addRow(this->createCheckBox(
            "Enable image uploader", getSettings()->imageUploaderEnabled));
        groupLayout->addRow(
            this->createCheckBox("Ask for confirmation when uploading an image",
                                 getSettings()->askOnImageUpload));

        groupLayout->addRow(
            "Request URL: ",
            this->createLineEdit(getSettings()->imageUploaderUrl));
        groupLayout->addRow(
            "Form field: ",
            this->createLineEdit(getSettings()->imageUploaderFormField));
        groupLayout->addRow(
            "Extra Headers: ",
            this->createLineEdit(getSettings()->imageUploaderHeaders));
        groupLayout->addRow(
            "Image link: ",
            this->createLineEdit(getSettings()->imageUploaderLink));
        groupLayout->addRow(
            "Deletion link: ",
            this->createLineEdit(getSettings()->imageUploaderDeletionLink));
    }
}

}  // namespace chatterino
