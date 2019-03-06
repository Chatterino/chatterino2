#include "NotificationPage.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "singletons/Settings.hpp"
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

namespace chatterino
{
    NotificationPage::NotificationPage()
        : SettingsPage("Notifications", ":/settings/notification2.svg")
    {
        LayoutCreator<NotificationPage> layoutCreator(this);

        auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
        {
            auto tabs = layout.emplace<QTabWidget>();
            {
                auto settings = tabs.appendTab(new QVBoxLayout, "Options");
                {
                    settings.emplace<QLabel>("Enable for selected channels");
                    settings.append(this->createCheckBox("Flash taskbar",
                        getSettings()->notificationFlashTaskbar));
                    settings.append(this->createCheckBox(
                        "Play sound", getSettings()->notificationPlaySound));
#ifdef Q_OS_WIN
                    settings.append(this->createCheckBox(
                        "Enable toasts (Windows 8 or later)",
                        getSettings()->notificationToast));
#endif
                    auto customSound =
                        layout.emplace<QHBoxLayout>().withoutMargin();
                    {
                        customSound.append(this->createCheckBox("Custom sound",
                            getSettings()->notificationCustomSound));
                        auto selectFile = customSound.emplace<QPushButton>(
                            "Select custom sound file");
                        QObject::connect(selectFile.getElement(),
                            &QPushButton::clicked, this, [this] {
                                auto fileName = QFileDialog::getOpenFileName(
                                    this, tr("Open Sound"), "",
                                    tr("Audio Files (*.mp3 *.wav)"));
                                getSettings()->notificationPathSound = fileName;
                            });
                    }

                    settings->addStretch(1);
                }
                auto twitchChannels = tabs.appendTab(new QVBoxLayout, "Twitch");
                {
                    EditableModelView* view =
                        twitchChannels
                            .emplace<EditableModelView>(
                                getApp()->notifications->createModel(
                                    nullptr, Platform::Twitch))
                            .getElement();
                    view->setTitles({"Twitch channels"});

                    view->getTableView()
                        ->horizontalHeader()
                        ->setSectionResizeMode(QHeaderView::Fixed);
                    view->getTableView()
                        ->horizontalHeader()
                        ->setSectionResizeMode(0, QHeaderView::Stretch);

                    QTimer::singleShot(1, [view] {
                        view->getTableView()->resizeColumnsToContents();
                        view->getTableView()->setColumnWidth(0, 200);
                    });

                    view->addButtonPressed.connect([] {
                        getApp()
                            ->notifications->channelMap[Platform::Twitch]
                            .appendItem("channel");
                    });
                }
                /*
                auto mixerChannels = tabs.appendTab(new QVBoxLayout, "Mixer");
                {
                    EditableModelView *view =
                        mixerChannels
                            .emplace<EditableModelView>(
                                getApp()->notifications->createModel(
                                    nullptr, Platform::Mixer))
                            .getElement();
                    view->setTitles({"Mixer channels"});

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
                            ->notifications->channelMap[Platform::Mixer]
                            .appendItem("channel");
                    });
                }
                */
            }
        }
    }
}  // namespace chatterino
