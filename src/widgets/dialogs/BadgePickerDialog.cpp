#include "BadgePickerDialog.hpp"
#include <QSizePolicy>
#include "singletons/Resources.hpp"

#include "common/GlobalBadges.hpp"

namespace chatterino {

BadgePickerDialog::BadgePickerDialog(QList<DisplayBadge> badges,
                                     QWidget *parent)
    : QDialog(parent)
    , vbox_(this)
    , okButton_("OK")
    , cancelButton_("Cancel")
    , badges_(badges)
{
    this->vbox_.addWidget(&dropdown_);
    this->vbox_.addLayout(&buttonBox_);

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

    // this has to be delayed so the constructor returns
    // before we try to get a weak_ptr ref to this in initializeValues()
    QTimer::singleShot(1, this, [this] { this->initializeValues(); });
}

void BadgePickerDialog::initializeValues()
{
    for (const auto &item : this->badges_)
    {
        this->dropdown_.addItem(item.displayName(), item.identifier());
    }

    GlobalBadges::instance()->getBadgeIcons(
        this->badges_,
        [weak = weakOf(this)](QString identifier, const QIconPtr icon) {
            auto shared = weak.lock();
            if (!shared)
                return;

            int index = shared->dropdown_.findData(identifier);
            if (index != -1)
            {
                shared->dropdown_.setItemIcon(index, *icon);
            }
        });
}

boost::optional<DisplayBadge> BadgePickerDialog::getSelection() const
{
    const auto i = this->dropdown_.currentIndex();
    if (i >= 0 && i < this->badges_.size())
    {
        return this->badges_[i];
    }

    return boost::none;
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
