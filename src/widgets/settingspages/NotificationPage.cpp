#include "NotificationPage.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Toasts.hpp"
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
                settings.append(this->createCheckBox(
                    "Flash taskbar", getSettings()->notificationFlashTaskbar));
                settings.append(this->createCheckBox(
                    "Play sound", getSettings()->notificationPlaySound));
#ifdef Q_OS_WIN
                settings.append(
                    this->createCheckBox("Enable toasts (Windows 8 or later)",
                                         getSettings()->notificationToast));
                auto openIn = settings.emplace<QHBoxLayout>().withoutMargin();
                {
                    openIn.emplace<QLabel>("Open stream from Toast:  ")
                        ->setSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred);

                    // implementation of custom combobox done
                    // because addComboBox only can handle strings-settings
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
                auto customSound =
                    layout.emplace<QHBoxLayout>().withoutMargin();
                {
                    customSound.append(this->createCheckBox(
                        "Custom sound",
                        getSettings()->notificationCustomSound));
                    auto selectFile = customSound.emplace<QPushButton>(
                        "Select custom sound file");
                    QObject::connect(
                        selectFile.getElement(), &QPushButton::clicked, this,
                        [this] {
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
                EditableModelView *view =
                    twitchChannels
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
QComboBox *NotificationPage::createToastReactionComboBox(
    std::vector<pajlada::Signals::ScopedConnection> managedConnections)
{
    QComboBox *toastReactionOptions = new QComboBox();

    for (int i = 0; i <= static_cast<int>(ToastReactions::DontOpen); i++)
    {
        toastReactionOptions->insertItem(
            i, Toasts::findStringFromReaction(static_cast<ToastReactions>(i)));
    }

    // update when setting changes
    pajlada::Settings::Setting<int> setting = getSettings()->openFromToast;
    setting.connect(
        [toastReactionOptions](const int &index, auto) {
            toastReactionOptions->setCurrentIndex(index);
        },
        managedConnections);

    QObject::connect(toastReactionOptions, &QComboBox::currentTextChanged,
                     [setting](const QString &newValue) {
                         getSettings()->openFromToast.setValue(static_cast<int>(
                             Toasts::findReactionFromString(newValue)));
                     });

    return toastReactionOptions;
}
}  // namespace chatterino
