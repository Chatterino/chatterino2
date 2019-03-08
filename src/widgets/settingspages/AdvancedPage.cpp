#include "AdvancedPage.hpp"

#include "Application.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "controllers/taggedusers/TaggedUsersModel.hpp"
#include "singletons/Logging.hpp"
#include "singletons/Paths.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

namespace chatterino
{
    AdvancedPage::AdvancedPage()
        : SettingsPage("Advanced", ":/settings/advanced.svg")
    {
        auto app = getApp();
        LayoutCreator<AdvancedPage> layoutCreator(this);

        auto tabs = layoutCreator.emplace<QTabWidget>();

        {
            auto layout = tabs.appendTab(new QVBoxLayout, "Cache");
            auto folderLabel = layout.emplace<QLabel>();

            folderLabel->setTextFormat(Qt::RichText);
            folderLabel->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                                 Qt::LinksAccessibleByKeyboard |
                                                 Qt::LinksAccessibleByKeyboard);
            folderLabel->setOpenExternalLinks(true);

            getSettings()->cachePath.connect(
                [folderLabel](const auto&, auto) mutable {
                    QString newPath = getPaths()->cacheDirectory();

                    QString pathShortened =
                        "Cache saved at <a href=\"file:///" + newPath +
                        "\"><span style=\"color: white;\">" +
                        shortenString(newPath, 50) + "</span></a>";

                    folderLabel->setText(pathShortened);
                    folderLabel->setToolTip(newPath);
                });

            layout->addStretch(1);

            auto selectDir =
                layout.emplace<QPushButton>("Set custom cache folder");

            QObject::connect(
                selectDir.getElement(), &QPushButton::clicked, this, [this] {
                    auto dirName = QFileDialog::getExistingDirectory(this);

                    getSettings()->cachePath = dirName;
                });

            auto resetDir =
                layout.emplace<QPushButton>("Reset custom cache folder");
            QObject::connect(resetDir.getElement(), &QPushButton::clicked, this,
                []() mutable {
                    getSettings()->cachePath = "";  //
                });

            // Logs end
        }
    }
}  // namespace chatterino
