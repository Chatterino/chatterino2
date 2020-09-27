#include "NotificationPage.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Toasts.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QCheckBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QTimer>

namespace chatterino {

NotificationPage::NotificationPage()
{
    LayoutCreator<NotificationPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        auto tabs = layout.emplace<QTabWidget>();
        {
            auto settings = tabs.appendTab(new QVBoxLayout, "Options");
            {
                settings.emplace<QLabel>(
                    "You can be informed when certain channels go live. You "
                    "can be informed in multiple ways:");

                settings.append(this->createCheckBox(
                    "Flash taskbar", getSettings()->notificationFlashTaskbar));
//        auto notification = layout.emplace<QGroupBox>("Notification layout")
//                                .setLayoutType<QVBoxLayout>();
//
//        notification.append(this->createCheckBox(
//            "Flash taskbar", getSettings()->notificationFlashTaskbar));
//        auto soundSettings =
//            notification.emplace<QHBoxLayout>().withoutMargin();
//        soundSettings.append(this->createCheckBox(
//            "Play sound", getSettings()->notificationSound));
//
//        auto selectFile =
//            soundSettings.emplace<QPushButton>("Select custom sound file");
//        auto soundReset = soundSettings.emplace<QPushButton>("Reset");
//        auto filePath = soundSettings.emplace<QLabel>();
//
//        soundSettings->addStretch();
//
//        QObject::connect(selectFile.getElement(), &QPushButton::clicked, this,
//                         [this] {
//                             auto fileName = QFileDialog::getOpenFileName(
//                                 this, tr("Open Sound"), "",
//                                 tr("Audio Files (*.mp3 *.wav)"));
//                             getSettings()->notificationSoundUrl = fileName;
//                         });
//
//        QObject::connect(soundReset.getElement(), &QPushButton::clicked,
//                         [] { getSettings()->notificationSoundUrl = ""; });
//
//        // Url at the end
//        getSettings()->notificationSoundUrl.connect(
//            [filePath](const QString &soundUrl, auto) mutable {
//                QString pathOriginal = soundUrl;
//
//                QString pathShortened = "<a href=\"file:///" + pathOriginal +
//                                        "\"><span style=\"color: white;\">" +
//                                        shortenString(pathOriginal, 50) +
//                                        "</span></a>";
//
//                filePath->setText(pathShortened);
//                filePath->setToolTip(pathOriginal);
//            });
//
//        filePath->setTextFormat(Qt::RichText);
//        filePath->setTextInteractionFlags(Qt::TextBrowserInteraction |
//                                          Qt::LinksAccessibleByKeyboard);
//        filePath->setOpenExternalLinks(true);
#ifdef Q_OS_WIN
                notification.append(this->createCheckBox(
                    "Show notification", getSettings()->notificationToast));
                auto openIn =
                    notification.emplace<QHBoxLayout>().withoutMargin();
                {
                    openIn
                        .emplace<QLabel>(
                            "Action when clicking on a notification:  ")
                        ->setSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred);

                    // implementation of custom combobox done
                    // because addComboBox only can handle strings-layout
                    // int setting for the ToastReaction is desired
                    openIn
                        .append(this->createToastReactionComboBox(
                            this->managedConnections_))
                        ->setSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred);
                }
                openIn->setContentsMargins(40, 0, 0, 0);
                openIn->setSizeConstraint(QLayout::SetMaximumSize);
#endif
            }
            layout->addSpacing(16);

            {
                auto channels =
                    layout
                        .emplace<QGroupBox>(
                            "Channels you want to be notified about")
                        .setLayoutType<QVBoxLayout>();
                EditableModelView *view =
                    channels
                        .emplace<EditableModelView>(
                            getApp()->notifications->createModel(
                                nullptr, Platform::Twitch))
                        .getElement();
                view->setTitles({"Twitch channels"});

                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()
                        ->notifications->channelMap[Platform::Twitch]
                        .append("channel");
                });
            }
        }
        QComboBox *NotificationPage::createToastReactionComboBox(
            std::vector<pajlada::Signals::ScopedConnection> managedConnections)
        {
            QComboBox *toastReactionOptions = new QComboBox();

            for (int i = 0; i <= static_cast<int>(ToastReaction::DontOpen); i++)
            {
                toastReactionOptions->insertItem(
                    i, Toasts::findStringFromReaction(
                           static_cast<ToastReaction>(i)));
            }

            // update when setting changes
            pajlada::Settings::Setting<int> setting =
                getSettings()->openFromToast;
            setting.connect(
                [toastReactionOptions](const int &index, auto) {
                    toastReactionOptions->setCurrentIndex(index);
                },
                managedConnections);

            QObject::connect(
                toastReactionOptions,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                [](const int &newValue) {
                    getSettings()->openFromToast.setValue(newValue);
                });

            return toastReactionOptions;
        }
    }  // namespace chatterino
