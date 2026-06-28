#include "controllers/highlights/ConfigureDialog.hpp"

#include "Application.hpp"
#include "controllers/highlights/types/Common.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "util/DisplayBadge.hpp"
#include "util/Variant.hpp"
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/color/ColorButton.hpp"

#include <qboxlayout.h>
#include <qcheckbox.h>
#include <QComboBox>
#include <QDesktopServices>
#include <qdialogbuttonbox.h>
#include <QFileDialog>
#include <qformlayout.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmenu.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <QSignalBlocker>
#include <qsizepolicy.h>
#include <qtextedit.h>
#include <QToolButton>

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

using namespace Qt::StringLiterals;

namespace {

// Add additional badges for highlights here
const QList<DisplayBadge> AVAILABLE_BADGES = {
    {"Broadcaster", "broadcaster"},
    {"Admin", "admin"},
    {"Staff", "staff"},
    {"Moderator", "moderator"},
    {"Lead Moderator", "lead_moderator"},
    {"Verified", "partner"},
    {"VIP", "vip"},
    {"Founder", "founder"},
    {"Subscriber", "subscriber"},
    {"Predicted Blue", "predictions/blue-1,predictions/blue-2"},
    {"Predicted Pink", "predictions/pink-2,predictions/pink-1"},
};

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
          },
          parent)
    , data(std::move(_data))
{
    // TODO: THIS DIALOG IS INVISIBLE IN LIGHT THEME
    this->setWindowTitle(u"Chatterino - Highlight editor"_s);
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->resize(515, 500);

    auto *dialogLayout = new QVBoxLayout;

    // TODO
    // An enabled toggle
    // Is "Active" a better phrase for this?

    // TODO
    // The highlight pattern
    // The default case should preferably always be text matching
    // We should provide some functionality underneath this or nearby this to highlight on badges I think

    // this->setLayout(layout);
    auto *formLayout = new QFormLayout;

#ifndef NDEBUG
    {
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
        formLayout->addRow("Enabled", w);
    }

    {
        auto value = std::visit(
            [](auto &&h) {
                return h.name;
            },
            this->data);
        auto *w = new QLineEdit();
        formLayout->addRow("Name", w);

        auto defaultName = getDefaultName(this->data);

        std::visit(
            [w, defaultName](auto &&h) {
                w->setPlaceholderText(defaultName);
                w->setText(h.name);

                QObject::connect(w, &QLineEdit::textChanged,
                                 [&](const auto &newText) {
                                     h.name = newText;
                                 });
            },
            this->data);
    }

    std::visit(
        variant::Overloaded{
            [formLayout](FilterHighlight &h) {
                auto *errorLabel = new QLabel(h.getError());
                auto *w = new QLineEdit();
                w->setText(h.filterText);
                QObject::connect(w, &QLineEdit::textChanged,
                                 [errorLabel, &h](const auto &newText) {
                                     h.setFilterText(newText);
                                     // TODO: Make the error display prettier?
                                     errorLabel->setText(h.getError());
                                 });
                formLayout->addRow("Filter", w);
                formLayout->addRow("", errorLabel);
            },
            [formLayout](BadgeHighlight &h) {
                auto *w = new QComboBox();
                for (const auto &item : AVAILABLE_BADGES)
                {
                    w->addItem(item.displayName(), item.badgeName());
                }

                QObject::connect(
                    w, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [&h](int index) {
                        std::optional<DisplayBadge> badge;
                        if (index >= 0 && index < AVAILABLE_BADGES.size())
                        {
                            badge = AVAILABLE_BADGES[index];
                            h.setBadgeName(badge->badgeName());
                            h.setDisplayName(badge->displayName());
                        }
                    });

                w->setCurrentText(h.displayName);

                getApp()->getTwitchBadges()->getBadgeIcons(
                    AVAILABLE_BADGES,
                    [w](QString identifier, const std::shared_ptr<QIcon> icon) {
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

    // TODO
    // Group of side effects
    //  - Show message in mentions
    //  - Highlight message
    //  - Flash taskbar
    //  - Not implemented but highlight portion of message?
    //  - Play sound (default sound? custom sound url?)
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
                // TODO: confirm colorButton & setting are never deleted and the signal is deleted
                // once the dialog is closed
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
                // dialog->setWindowModality(Qt::WindowModality::ApplicationModal);
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
            auto *lbl = new QLabel("Play sound");
            auto *w = new QCheckBox;
            w->setChecked(shouldPlaySound(this->data));

            QObject::connect(w, &QCheckBox::checkStateChanged,
                             [&](auto checkstate) {
                                 std::visit(
                                     [checkstate](auto &&h) {
                                         h.outcome.playSound = checkstate;
                                     },
                                     this->data);
                             });
            l->addRow(lbl, w);

            addSettingMenu(lbl, w, [this, w] {
                std::visit(
                    [](auto &&h) {
                        h.outcome.playSound = std::nullopt;
                    },
                    this->data);
                w->setChecked(shouldPlaySound(this->data));
            });
        }

        {
            auto *ll = new QHBoxLayout;

            auto *lbl = new QLabel("Custom sound URL");

            auto value = std::visit(
                [](auto &&h) {
                    return h.outcome.customSoundURL;
                },
                this->data);

            auto *soundURLLabel = new QLabel(value.toLocalFile());
            ll->addWidget(soundURLLabel);

            auto *editAction = new QAction("Set custom sound");
            QObject::connect(editAction, &QAction::triggered,
                             [this, soundURLLabel] {
                                 auto fileUrl = QFileDialog::getOpenFileUrl(
                                     this, tr("Open Sound"), QUrl(),
                                     tr("Audio Files (*.mp3 *.wav)"));
                                 soundURLLabel->setText(fileUrl.toLocalFile());
                                 std::visit(
                                     [fileUrl](auto &&h) {
                                         h.outcome.customSoundURL = fileUrl;
                                     },
                                     this->data);
                             });

            auto *edit = new QToolButton;
            edit->setToolTip("Set custom sound");
            edit->setIcon(QIcon(":/buttons/edit.svg"));
            ll->addWidget(edit);
            QObject::connect(edit, &QToolButton::clicked, editAction,
                             &QAction::triggered);

            auto *clearAction = new QAction("Clear custom sound");
            QObject::connect(clearAction, &QAction::triggered,
                             [this, soundURLLabel] {
                                 soundURLLabel->setText({});
                                 std::visit(
                                     [](auto &&h) {
                                         h.outcome.customSoundURL = QUrl{};
                                     },
                                     this->data);
                             });
            auto *clear = new QToolButton;
            clear->setToolTip("Clear custom sound");
            clear->setIcon(QIcon(":/buttons/cancel.svg"));
            ll->addWidget(clear);
            QObject::connect(clear, &QToolButton::clicked, clearAction,
                             &QAction::triggered);

            l->addRow(lbl);
            l->addRow(ll);

            lbl->setContextMenuPolicy(
                Qt::ContextMenuPolicy::ActionsContextMenu);
            lbl->addAction(editAction);
            lbl->addAction(clearAction);
            soundURLLabel->setContextMenuPolicy(
                Qt::ContextMenuPolicy::ActionsContextMenu);
            soundURLLabel->addAction(editAction);
            soundURLLabel->addAction(clearAction);
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
