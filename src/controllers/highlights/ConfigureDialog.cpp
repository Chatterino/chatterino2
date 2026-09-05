// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/ConfigureDialog.hpp"

#include "Application.hpp"
#include "controllers/highlights/Sounds.hpp"
#include "controllers/highlights/types/Common.hpp"
#include "controllers/sound/ISoundController.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "util/DisplayBadge.hpp"
#include "util/Variant.hpp"
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/color/ColorButton.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace chatterino::highlights {

template <typename T>
concept SupportsCaseSensitivity = requires(T a) {
    { a.isCaseSensitive() } -> std::same_as<bool>;
    { a.setCaseSensitive(true) };
};

template <typename T>
concept SupportsRegex = requires(T a) {
    { a.isRegex() } -> std::same_as<bool>;
    { a.setRegex(true) };
};

template <typename T>
concept SupportsDefaultName = requires(T a) {
    { a.getDefaultName() } -> std::same_as<QString>;
};

template <typename T>
concept SupportsGetID = requires(T a) {
    { a.getID() } -> std::same_as<QStringView>;
};

template <typename T>
concept HasDescription = requires {
    { T::DESCRIPTION } -> std::convertible_to<QStringView>;
};

template <typename T>
concept HasCustomizableName = requires(T a) {
    { a.name } -> std::convertible_to<QString>;
};

namespace {

auto makeCheckbox(bool &value)
{
    auto *w = new QCheckBox();
    w->setChecked(value);

    QObject::connect(w, &QCheckBox::checkStateChanged, [&](auto checkstate) {
        value = checkstate == Qt::CheckState::Checked;
    });
    return w;
}

auto makeLineEdit(QString &value)
{
    auto *w = new QLineEdit();
    w->setText(value);
    QObject::connect(w, &QLineEdit::textChanged, [&](const auto &newText) {
        value = newText;
    });
    return w;
}

template <typename ResetToDefaultFunctor>
void addSettingMenu(QWidget *lbl, QWidget *w,
                    ResetToDefaultFunctor handleResetToDefault)
{
    lbl->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    w->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);

    auto *resetToDefault = new QAction("Reset to default");
    QObject::connect(resetToDefault, &QAction::triggered, handleResetToDefault);
    lbl->addAction(resetToDefault);
    w->addAction(resetToDefault);
}

}  // namespace

ConfigureDialog::ConfigureDialog(AllHighlights _data, QWidget *parent)
    : BasePopup(
          {
              BaseWindow::EnableCustomFrame,
              BaseWindow::DisableLayoutSave,
              BaseWindow::BoundsCheckOnShow,
              BaseWindow::UseSettingsStylesheet,
          },
          parent)
    , data(std::move(_data))
{
    this->setWindowTitle(u"Chatterino - Highlight editor"_s);
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->resize(515, 500);

    auto *dialogLayout = new QVBoxLayout;

    std::visit(variant::Overloaded{
                   [dialogLayout](HasDescription auto &h) {
                       using ActualType = std::decay_t<decltype(h)>;

                       auto *w = new QLabel(ActualType::DESCRIPTION.toString());
                       w->setOpenExternalLinks(true);
                       w->setSizePolicy(QSizePolicy::Minimum,
                                        QSizePolicy::Fixed);

                       dialogLayout->addWidget(w);
                   },
                   [](auto &&) {},
               },
               this->data);

    auto *formLayout = new QFormLayout;

#ifndef NDEBUG
    {
        // When in debug mode, we also show the highlights ID. Not sure if that can be handy for everyone
        auto value = std::visit(variant::Overloaded{
                                    [](SupportsGetID auto &h) {
                                        return h.getID();
                                    },
                                    [](auto &&h) {
                                        using ActualType =
                                            std::decay_t<decltype(h)>;
                                        return ActualType::ID;
                                    },
                                },
                                this->data);
        auto *w = new QLabel(value.toString());
        w->setTextInteractionFlags(Qt::TextSelectableByMouse);
        formLayout->addRow("ID", w);
    }
#endif

    {
        auto *lbl = new QLabel("Enabled");
        auto *w = new QCheckBox;
        w->setChecked(highlights::isEnabled(this->data));

        QObject::connect(w, &QCheckBox::checkStateChanged,
                         [&](auto checkstate) {
                             std::visit(
                                 [checkstate](auto &&h) {
                                     h.enabled = checkstate;
                                 },
                                 this->data);
                         });
        formLayout->addRow(lbl, w);

        addSettingMenu(lbl, w, [this, w] {
            std::visit(
                [](auto &&h) {
                    h.enabled = std::nullopt;
                },
                this->data);
            QSignalBlocker signalBlocker(w);
            w->setChecked(highlights::isEnabled(this->data));
        });
    }

    auto *nameWidget = new QLineEdit();

    {
        auto defaultName = getDefaultName(this->data);

        std::visit(variant::Overloaded{
                       [formLayout, defaultName,
                        w{nameWidget}](HasCustomizableName auto &h) {
                           formLayout->addRow("Name", w);
                           w->setPlaceholderText(defaultName);
                           w->setText(h.name);

                           QObject::connect(w, &QLineEdit::textChanged,
                                            [&](const auto &newText) {
                                                h.name = newText;
                                            });
                       },
                       [formLayout](auto &&h) {
                           using ActualType = std::decay_t<decltype(h)>;
                           formLayout->addRow(
                               "Name",
                               new QLabel(ActualType::DEFAULT_NAME.toString()));
                       },
                   },
                   this->data);
    }

    // Pattern / Badge / User / Filter
    std::visit(
        variant::Overloaded{
            [formLayout](FilterHighlight &h) {
                auto *errorLabel = new QLabel(h.getError());
                auto *w = new QLineEdit();
                w->setText(h.filterText);
                QObject::connect(w, &QLineEdit::textChanged,
                                 [errorLabel, &h](const auto &newText) {
                                     h.setFilterText(newText);
                                     errorLabel->setText(h.getError());
                                 });
                formLayout->addRow("Filter", w);
                formLayout->addRow(errorLabel);
            },
            [formLayout, nameWidget](BadgeHighlight &h) {
                auto *w = new QComboBox();
                for (const auto &item : highlights::twitchBadges())
                {
                    w->addItem(item.displayName(), item.badgeName());
                }

                QObject::connect(
                    w, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [&h, nameWidget](int index) {
                        std::optional<DisplayBadge> badge;
                        if (index >= 0 &&
                            index < highlights::twitchBadges().size())
                        {
                            badge = highlights::twitchBadges()[index];
                            h.setBadgeName(badge->badgeName());
                            nameWidget->setPlaceholderText(h.getDefaultName());
                        }
                    });

                w->setCurrentText(h.getDefaultName());

                getApp()->getTwitchBadges()->getBadgeIcons(
                    highlights::twitchBadges(),
                    [w](const QString &identifier,
                        const std::shared_ptr<QIcon> &icon) {
                        if (!w)
                        {
                            return;
                        }

                        int index = w->findData(identifier);
                        if (index != -1)
                        {
                            w->setItemIcon(index, *icon);
                        }
                    });
                formLayout->addRow("Badge", w);
            },
            [formLayout](UserHighlight &h) {
                auto *w = new QLineEdit();
                w->setText(h.username);
                QObject::connect(w, &QLineEdit::textChanged,
                                 [&](const auto &newText) {
                                     h.username = newText;
                                 });
                formLayout->addRow("Username", w);
            },
            [formLayout](MessageHighlight &h) {
                auto *w = new QLineEdit();
                w->setText(h.pattern);
                QObject::connect(w, &QLineEdit::textChanged,
                                 [&](const auto &newText) {
                                     h.setPattern(newText);
                                 });
                formLayout->addRow("Pattern", w);
            },
            [formLayout](AnnouncementsHighlight &h) {
                auto *lbl = new QLabel("Override colored announcements");
                lbl->setToolTip(
                    "If enabled, colored announcements will show up with your "
                    "selected Announcements highlight color.");
                auto *w = new QCheckBox();
                w->setChecked(h.overrideColoredAnnouncements.value_or(
                    AnnouncementsHighlight::
                        OVERRIDE_COLORED_ANNOUNCEMENTS_DEFAULT));
                addSettingMenu(lbl, w, [w, &h] {
                    h.overrideColoredAnnouncements = std::nullopt;
                    QSignalBlocker signalBlocker(w);
                    w->setChecked(h.overrideColoredAnnouncements.value_or(
                        AnnouncementsHighlight::
                            OVERRIDE_COLORED_ANNOUNCEMENTS_DEFAULT));
                });
                QObject::connect(
                    w, &QCheckBox::checkStateChanged, [&](auto checkstate) {
                        h.overrideColoredAnnouncements = checkstate;
                    });
                formLayout->addRow(lbl, w);
            },
            [&](auto &&) {
                //default xd
            },
        },
        this->data);

    std::visit(variant::Overloaded{
                   [&](SupportsRegex auto &h) {
                       auto *lbl = new QLabel("Regex");
                       auto *w = new QCheckBox;
                       w->setChecked(h.isRegex());
                       QObject::connect(w, &QCheckBox::checkStateChanged,
                                        [&](auto checkstate) {
                                            h.setRegex(checkstate);
                                        });
                       formLayout->addRow(lbl, w);

                       addSettingMenu(lbl, w, [w, &h] {
                           h.setRegex(std::nullopt);
                           QSignalBlocker signalBlocker(w);
                           w->setChecked(h.isRegex());
                       });

                       // TODO: Should this have some sort of indicator that it's going to open your browser?
                       auto *regexHelp = new QAction("Regex help");
                       QObject::connect(regexHelp, &QAction::triggered, [] {
                           QDesktopServices::openUrl(
                               u"https://wiki.chatterino.com/Regex/"_s);
                       });
                       lbl->addAction(regexHelp);
                       w->addAction(regexHelp);
                   },
                   [&](auto &&) {},
               },
               this->data);

    std::visit(variant::Overloaded{
                   [&](SupportsCaseSensitivity auto &h) {
                       auto *lbl = new QLabel("Case sensitive");
                       auto *w = new QCheckBox;
                       w->setChecked(h.isCaseSensitive());
                       QObject::connect(w, &QCheckBox::checkStateChanged,
                                        [&](auto checkstate) {
                                            h.setCaseSensitive(checkstate);
                                        });
                       formLayout->addRow(lbl, w);

                       addSettingMenu(lbl, w, [w, &h] {
                           h.setCaseSensitive(std::nullopt);
                           QSignalBlocker signalBlocker(w);
                           w->setChecked(h.isCaseSensitive());
                       });
                   },
                   [&](auto &&) {},
               },
               this->data);

    dialogLayout->addLayout(formLayout);

    {
        auto *group = new QGroupBox("Side effects");

        auto *l = new QFormLayout;
        {
            auto *lbl = new QLabel("Show message in mentions");
            auto *w = new QCheckBox;
            w->setChecked(shouldShowInMentions(this->data));

            QObject::connect(w, &QCheckBox::checkStateChanged,
                             [&](auto checkstate) {
                                 std::visit(
                                     [checkstate](auto &&h) {
                                         h.outcome.showInMentions = checkstate;
                                     },
                                     this->data);
                             });
            l->addRow(lbl, w);

            addSettingMenu(lbl, w, [this, w] {
                std::visit(
                    [](auto &&h) {
                        h.outcome.showInMentions = std::nullopt;
                    },
                    this->data);
                w->setChecked(shouldShowInMentions(this->data));
            });
        }

        {
            auto *lbl = new QLabel("Flash taskbar");
            auto *w = new QCheckBox;
            w->setChecked(shouldAlert(this->data));

            QObject::connect(w, &QCheckBox::checkStateChanged,
                             [&](auto checkstate) {
                                 std::visit(
                                     [checkstate](auto &&h) {
                                         h.outcome.alert = checkstate;
                                     },
                                     this->data);
                             });
            l->addRow(lbl, w);

            addSettingMenu(lbl, w, [this, w] {
                std::visit(
                    [](auto &&h) {
                        h.outcome.alert = std::nullopt;
                    },
                    this->data);
                w->setChecked(shouldAlert(this->data));
            });
        }

        {
            auto *lbl = new QLabel("Background color");
            auto *w = new ColorButton(*getBackgroundColor(this->data));
            w->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

            QObject::connect(w, &ColorButton::clicked, [this, w]() {
                auto *dialog =
                    new ColorPickerDialog(*getBackgroundColor(this->data), w);
                QObject::connect(
                    dialog, &ColorPickerDialog::colorConfirmed, w,
                    [this, w](auto selected) {
                        if (selected.isValid())
                        {
                            w->setColor(selected);
                            std::visit(
                                [selected](auto &&h) {
                                    h.outcome.setBackgroundColor(selected);
                                },
                                this->data);
                        }
                    });
                dialog->show();
            });
            l->addRow(lbl, w);

            addSettingMenu(lbl, w, [this, w] {
                std::visit(
                    [](auto &&h) {
                        h.outcome.setBackgroundColor(std::nullopt);
                    },
                    this->data);
                w->setColor(*getBackgroundColor(this->data));
            });

            auto *disableBackgroundColor =
                new QAction("Disable background color");
            QObject::connect(disableBackgroundColor, &QAction::triggered,
                             [this, w] {
                                 std::visit(
                                     [](auto &&h) {
                                         h.outcome.setBackgroundColor(QColor{});
                                     },
                                     this->data);
                                 w->setColor(QColor{});
                             });
            lbl->addAction(disableBackgroundColor);
            w->addAction(disableBackgroundColor);
        }

        {
            // SOUND V2

            auto *w = new QComboBox();
            auto currentSound = highlights::getSound(this->data);
            auto *soundURLLabel = new QLabel;

            w->addItem("None", u""_s);  // empty string = disable sound
            for (const auto &[soundID, defaultSound] :
                 highlights::defaultSounds())
            {
                w->addItem(defaultSound.displayName,
                           QVariant::fromValue(defaultSound));
            }
            w->addItem("Custom sound...");

            int numRows = w->count();

            if (currentSound.isEmpty())
            {
                w->setCurrentText("None");
                soundURLLabel->setText("Not playing any sound");
            }
            else
            {
                if (auto d = highlights::resolveDefaultSound(currentSound))
                {
                    w->setCurrentText(d->displayName);
                    soundURLLabel->setText(u"Playing bill-tin " %
                                           d->displayName);
                }
                else
                {
                    QUrl currentSoundUrl(currentSound);
                    w->addItem(currentSoundUrl.fileName());
                    w->setCurrentText(currentSoundUrl.fileName());
                    soundURLLabel->setText(u"Playing custom " % currentSound);
                }
            }

            this->previousSoundIndex = w->currentIndex();

            QObject::connect(
                w, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, numRows, w, soundURLLabel](int index) {
                    auto data = w->currentData();
                    if (data.canConvert<highlights::DefaultSound>())
                    {
                        auto defaultSound =
                            data.value<highlights::DefaultSound>();
                        std::visit(
                            [&defaultSound](auto &&h) {
                                h.outcome.setSound(defaultSound.id);
                            },
                            this->data);
                        soundURLLabel->setText(u"Playing bill-tin " %
                                               defaultSound.displayName);
                        this->previousSoundIndex = index;
                        w->removeItem(numRows);
                    }
                    else if (auto sound = data.toString(); sound.isNull())
                    {
                        auto fileUrl = QFileDialog::getOpenFileUrl(
                            this, tr("Open Sound"), QUrl(),
                            tr("Audio Files (*.mp3 *.wav)"));
                        if (fileUrl.isValid())
                        {
                            this->previousSoundIndex = index;
                            // soundURLLabel->setText(fileUrl.toLocalFile());
                            std::visit(
                                [fileUrl](auto &&h) {
                                    h.outcome.setSound(fileUrl.toString());
                                },
                                this->data);
                            soundURLLabel->setText(u"Playing custom " %
                                                   fileUrl.toString());
                            // w->setCurrentText("AAAAAAAAA");
                            QSignalBlocker block(w);
                            w->removeItem(numRows);
                            w->addItem(fileUrl.fileName());
                            w->setCurrentIndex(numRows);
                        }
                        else
                        {
                            QSignalBlocker block(w);
                            w->setCurrentIndex(this->previousSoundIndex);
                        }
                    }
                    else
                    {
                        assert(sound.isEmpty());

                        std::visit(
                            [](auto &&h) {
                                h.outcome.setSound("");
                            },
                            this->data);
                        soundURLLabel->setText("Not playing any sound");
                        this->previousSoundIndex = index;
                        w->removeItem(numRows);
                    }
                });

            auto *playSoundLabel = new QLabel("Play sound");
            l->addRow(playSoundLabel, w);
            l->addRow(soundURLLabel);

            addSettingMenu(playSoundLabel, w, [w, soundURLLabel, this] {
                std::visit(
                    [](auto &&h) {
                        h.outcome.setSound({});
                    },
                    this->data);
                QSignalBlocker signalBlocker(w);
                const auto defaultSound =
                    highlights::getDefaultSound(this->data);
                if (defaultSound.isEmpty())
                {
                    w->setCurrentText("None");
                    soundURLLabel->setText("Not playing any sound");
                }
                else
                {
                    if (auto defaultSoundXD = resolveDefaultSound(defaultSound))
                    {
                        w->setCurrentText(defaultSoundXD->displayName);
                        soundURLLabel->setText("Playing bill-tin " %
                                               defaultSoundXD->displayName);
                    }
                }
            });

            /*
            // TODO: Implement per-highlight sound volume support!
            auto *slider = new QSlider(Qt::Orientation::Horizontal);
            slider->setTickPosition(QSlider::TickPosition::TicksBothSides);

            QObject::connect(slider, &QSlider::valueChanged, this,
                             [this](const auto &newValue) {
                                 std::visit(
                                     [newValue](auto &&h) {
                                         h.outcome.volume = newValue;
                                     },
                                     this->data);
                             });
            auto *volumeLabel = new QLabel("Volume");

            l->addRow(volumeLabel, slider);
            addSettingMenu(volumeLabel, slider, [slider, this] {
                std::visit(
                    [](auto &&h) {
                        h.outcome.volume = std::nullopt;
                    },
                    this->data);
                QSignalBlocker signalBlocker(slider);
                slider->setValue(100);
            });
            */

            auto *test = new QPushButton("Test playsound");
            QObject::connect(test, &QPushButton::pressed, this, [this] {
                std::visit(
                    [](auto &&h) {
                        getApp()->getSound()->play(h.outcome.soundURL);
                    },
                    this->data);
            });
            l->addRow(test);
        }

        group->setLayout(l);

        dialogLayout->addWidget(group);
    }

    auto *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
        Q_EMIT this->confirmed(this->data);
        this->close();
    });
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this,
                     &BasePopup::close);
    dialogLayout->addWidget(buttonBox, 0, Qt::AlignRight);

    this->setLayout(dialogLayout);
}

}  // namespace chatterino::highlights
