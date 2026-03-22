// SPDX-FileCopyrightText: 2020 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/SelectChannelFiltersDialog.hpp"

#include "controllers/filters/FilterRecord.hpp"
#include "singletons/Settings.hpp"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace chatterino {

SelectChannelFiltersDialog::SelectChannelFiltersDialog(
    const QList<QUuid> &previousSelection, bool previousAnyOf, QWidget *parent)
    : QDialog(parent)
    , anyOf_(previousAnyOf)
{
    auto *vbox = new QVBoxLayout(this);
    auto *itemVbox = new QVBoxLayout;
    auto *buttonBox = new QHBoxLayout;
    auto *okButton = new QPushButton("Ok");
    auto *cancelButton = new QPushButton("Cancel");

    auto *scrollAreaContent = new QWidget;
    scrollAreaContent->setLayout(itemVbox);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(scrollAreaContent);

    vbox->addWidget(scrollArea);

    auto *anyOfCheckbox = new QCheckBox("Match if any filter applies", this);
    anyOfCheckbox->setCheckState(previousAnyOf ? Qt::CheckState::Checked
                                               : Qt::CheckState::Unchecked);
    QObject::connect(anyOfCheckbox, &QCheckBox::stateChanged,
                     [this](int state) {
                         if (state == 0)
                         {
                             this->anyOf_ = false;
                         }
                         else
                         {
                             this->anyOf_ = true;
                         }
                     });
    vbox->addWidget(anyOfCheckbox);

    vbox->addLayout(buttonBox);

    buttonBox->addStretch(1);
    buttonBox->addWidget(okButton);
    buttonBox->addWidget(cancelButton);

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

    auto availableFilters = getSettings()->filterRecords.readOnly();

    if (availableFilters->size() == 0)
    {
        auto *text = new QLabel("No filters defined");
        itemVbox->addWidget(text);
    }
    else
    {
        for (const auto &f : *availableFilters)
        {
            auto *checkbox = new QCheckBox(f->getName(), this);
            bool alreadySelected = previousSelection.contains(f->getId());
            checkbox->setCheckState(alreadySelected
                                        ? Qt::CheckState::Checked
                                        : Qt::CheckState::Unchecked);
            if (alreadySelected)
            {
                this->currentSelection_.append(f->getId());
            }

            QObject::connect(checkbox, &QCheckBox::stateChanged,
                             [this, id = f->getId()](int state) {
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

bool SelectChannelFiltersDialog::getAnyOf() const
{
    return this->anyOf_;
}

}  // namespace chatterino
