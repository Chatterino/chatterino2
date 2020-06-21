#include "SelectChannelFiltersDialog.hpp"

#include "singletons/Settings.hpp"

namespace chatterino {

SelectChannelFiltersDialog::SelectChannelFiltersDialog(
    const QList<QUuid> &previousSelection, QWidget *parent)
    : QDialog(parent)
{
    auto vbox = new QVBoxLayout(this);
    auto itemVbox = new QVBoxLayout;
    auto buttonBox = new QHBoxLayout;
    auto okButton = new QPushButton("OK");
    auto cancelButton = new QPushButton("Cancel");

    vbox->addLayout(itemVbox);
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

    this->setWindowFlags(
        (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    auto availableFilters = getCSettings().filterRecords.readOnly();

    if (availableFilters->size() == 0)
    {
        auto text = new QLabel("No filters defined");
        itemVbox->addWidget(text);
    }
    else
    {
        for (const auto &f : *availableFilters)
        {
            auto checkbox = new QCheckBox(f.getName(), this);
            bool alreadySelected = previousSelection.contains(f.getId());
            checkbox->setCheckState(alreadySelected
                                        ? Qt::CheckState::Checked
                                        : Qt::CheckState::Unchecked);
            if (alreadySelected)
            {
                this->currentSelection_.append(f.getId());
            }

            QObject::connect(checkbox, &QCheckBox::stateChanged,
                             [this, id = f.getId()](int state) {
                                 if (state == 0)
                                 {
                                     this->currentSelection_.removeOne(id);
                                 }
                                 else
                                 {
                                     this->currentSelection_.append(id);
                                 }
                             });

            itemVbox->addWidget(checkbox);
        }
    }
}

const QList<QUuid> &SelectChannelFiltersDialog::getSelection() const
{
    return this->currentSelection_;
}

}  // namespace chatterino
