#include "widgets/dialogs/BadgePickerDialog.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "singletons/Resources.hpp"

#include <QDialogButtonBox>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace chatterino {

BadgePickerDialog::BadgePickerDialog(QList<DisplayBadge> badges,
                                     QWidget *parent)
    : QDialog(parent)
{
    this->dropdown_ = new QComboBox;
    auto *vbox = new QVBoxLayout(this);
    auto *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    vbox->addWidget(this->dropdown_);
    vbox->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, [this] {
        this->accept();
        this->close();
    });
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, [this] {
        this->reject();
        this->close();
    });

    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    this->setWindowFlags(
        (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    // Add items.
    for (const auto &item : badges)
    {
        this->dropdown_->addItem(item.displayName(), item.badgeName());
    }

    const auto updateBadge = [=, this](int index) {
        BadgeOpt badge;
        if (index >= 0 && index < badges.size())
        {
            badge = badges[index];
        }
        this->currentBadge_ = badge;
    };

    QObject::connect(this->dropdown_,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     updateBadge);
    updateBadge(0);

    // Set icons.
    getApp()->getTwitchBadges()->getBadgeIcons(
        badges,
        [&dropdown = this->dropdown_](QString identifier, const QIconPtr icon) {
            if (!dropdown)
            {
                return;
            }

            int index = dropdown->findData(identifier);
            if (index != -1)
            {
                dropdown->setItemIcon(index, *icon);
            }
        });
}

}  // namespace chatterino
