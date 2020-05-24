#include "BadgePickerDialog.hpp"
#include <QSizePolicy>
#include "singletons/Resources.hpp"

namespace chatterino {

BadgePickerDialog::BadgePickerDialog(QWidget *parent)
    : QDialog(parent)
    , vbox_(this)
    , okButton_("OK")
    , cancelButton_("Cancel")
{
    this->vbox_.addWidget(&dropdown_);
    this->vbox_.addLayout(&buttonBox_);
    this->initializeValues();
    this->buttonBox_.addStretch(1);
    this->buttonBox_.addWidget(&okButton_);
    this->buttonBox_.addWidget(&cancelButton_);

    QObject::connect(&this->okButton_, SIGNAL(clicked()), this,
                     SLOT(okButtonClicked()));
    QObject::connect(&this->cancelButton_, SIGNAL(clicked()), this,
                     SLOT(cancelButtonClicked()));

    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    this->setWindowFlags(
        (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
}

void BadgePickerDialog::initializeValues()
{
    using Badges = BadgePickerDialog::Badges;
    this->dropdown_.addItem(QIcon(getResources().twitch.broadcaster),
                            "Broadcaster", Badges::Broadcaster);
    this->dropdown_.addItem(QIcon(getResources().twitch.admin), "Admin",
                            Badges::Admin);
    this->dropdown_.addItem(QIcon(getResources().twitch.staff), "Staff",
                            Badges::Staff);
    this->dropdown_.addItem(QIcon(getResources().twitch.moderator), "Moderator",
                            Badges::Moderator);
    this->dropdown_.addItem(QIcon(getResources().twitch.verified), "Verified",
                            Badges::Verified);
    this->dropdown_.addItem(QIcon(getResources().twitch.vip), "VIP",
                            Badges::VIP);
    this->dropdown_.addItem(QIcon(getResources().twitch.globalmod),
                            "Global Moderator", Badges::GlobalModerator);
}

BadgePickerDialog::Badges BadgePickerDialog::getSelection() const
{
    return static_cast<BadgePickerDialog::Badges>(
        this->dropdown_.currentData().toInt());
}

void BadgePickerDialog::okButtonClicked()
{
    this->accept();
    this->close();
}

void BadgePickerDialog::cancelButtonClicked()
{
    this->reject();
    this->close();
}

}  // namespace chatterino
