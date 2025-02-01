#include "widgets/settingspages/NotificationPage.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "controllers/notifications/NotificationModel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Toasts.hpp"
#include "util/LayoutCreator.hpp"
#include "util/Twitch.hpp"
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
                settings.append(
                    this->createCheckBox("Play sound for selected channels",
                                         getSettings()->notificationPlaySound));
                settings.append(this->createCheckBox(
                    "Play sound for any channel going live",
                    getSettings()->notificationOnAnyChannel));

                settings.append(this->createCheckBox(
                    "Suppress live notifications on startup",
                    getSettings()->suppressInitialLiveNotification));
#ifdef Q_OS_WIN
                settings.append(this->createCheckBox(
                    "Show notification", getSettings()->notificationToast));
                auto openIn = settings.emplace<QHBoxLayout>().withoutMargin();
                {
                    openIn
                        .emplace<QLabel>(
                            "Action when clicking on a notification:  ")
                        ->setSizePolicy(QSizePolicy::Maximum,
                                        QSizePolicy::Preferred);

                    // implementation of custom combobox done
                    // because addComboBox only can handle strings-settings
                    // int setting for the ToastReaction is desired
                    openIn.append(this->createToastReactionComboBox())
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
            auto twitchChannels =
                tabs.appendTab(new QVBoxLayout, "Selected Channels");
            {
                twitchChannels.emplace<QLabel>(
                    "These are the channels for which you will be informed "
                    "when they go live:");

                this->view_ = twitchChannels
                                  .emplace<EditableModelView>(
                                      getApp()->getNotifications()->createModel(
                                          nullptr, Platform::Twitch))
                                  .getElement();
                view_->setTitles({"Twitch channels"});
                view_->setValidationRegexp(twitchUserNameRegexp());

                view_->getTableView()->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::Fixed);
                view_->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                QTimer::singleShot(1, [this] {
                    view_->getTableView()->resizeColumnsToContents();
                    view_->getTableView()->setColumnWidth(0, 200);
                });

                // We can safely ignore this signal connection since we own the view_
                std::ignore = view_->addButtonPressed.connect([] {
                    getApp()->getNotifications()->addChannelNotification(
                        "channel", Platform::Twitch);
                });
            }
        }
    }
}
QComboBox *NotificationPage::createToastReactionComboBox()
{
    QComboBox *toastReactionOptions = new QComboBox();

    for (int i = 0; i <= static_cast<int>(ToastReaction::DontOpen); i++)
    {
        toastReactionOptions->insertItem(
            i, Toasts::findStringFromReaction(static_cast<ToastReaction>(i)));
    }

    // update when setting changes
    pajlada::Settings::Setting<int> setting = getSettings()->openFromToast;
    setting.connect(
        [toastReactionOptions](const int &index, auto) {
            toastReactionOptions->setCurrentIndex(index);
        },
        this->managedConnections_);

    QObject::connect(toastReactionOptions,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [](const int &newValue) {
                         getSettings()->openFromToast.setValue(newValue);
                     });

    return toastReactionOptions;
}

bool NotificationPage::filterElements(const QString &query)
{
    std::array fields{0, 1};

    return view_->filterSearchResults(query, fields);
}

}  // namespace chatterino
