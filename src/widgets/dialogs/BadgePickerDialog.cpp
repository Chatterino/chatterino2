#include "BadgePickerDialog.hpp"
#include <QSizePolicy>
#include "singletons/Resources.hpp"

#include "common/GlobalBadges.hpp"

namespace chatterino {

BadgePickerDialog::BadgePickerDialog(QList<DisplayBadge> badges,
                                     QWidget *parent)
    : QDialog(parent)
{
    this->dropdown_ = new QComboBox;
    auto vbox = new QVBoxLayout(this);
    auto buttonBox = new QHBoxLayout;
    auto okButton = new QPushButton("OK");
    auto cancelButton = new QPushButton("Cancel");

    vbox->addWidget(this->dropdown_);
    vbox->addLayout(buttonBox);

    buttonBox->addStretch(1);
    buttonBox->addWidget(cancelButton);
    buttonBox->addWidget(okButton);

    QObject::connect(okButton, &QAbstractButton::clicked, [this] {
        this->accept();
        this->close();
    });
    QObject::connect(cancelButton, &QAbstractButton::clicked, [this] {
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
        this->dropdown_->addItem(item.displayName(), item.identifier());
    }

    const auto updateBadge = [=](int index) {
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
    GlobalBadges::instance()->getBadgeIcons(
        badges,
        [&dropdown = this->dropdown_](QString identifier, const QIconPtr icon) {
            if (!dropdown)
                return;

            int index = dropdown->findData(identifier);
            if (index != -1)
            {
                dropdown->setItemIcon(index, *icon);
            }
        });
}


}  // namespace chatterino
