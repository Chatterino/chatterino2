#include "widgets/settingspages/ImageUploaderPage.hpp"

#include "Application.hpp"
#include "singletons/imageuploader/ImageUploader.hpp"
#include "singletons/imageuploader/UploadedImageModel.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

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
        //auto *forget = buttons.emplace<QPushButton>().getElement();
        //forget->setText("Remove image from log");
        buttons->addStretch();

        auto *view = layout.emplace<QListView>().getElement();
        view->setViewMode(QListView::IconMode);
        view->setModel(model);
        view->setIconSize(QSize(96, 96));
        view->setLayoutMode(QListView::Batched);
        view->setMovement(QListView::Static);
        view->setUniformItemSizes(true);

        // without this prop qt throws everything into a single line
        view->setResizeMode(QListView::Adjust);

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
