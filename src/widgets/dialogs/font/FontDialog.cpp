// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/font/FontDialog.hpp"

#include "widgets/dialogs/font/FontFamilyWidget.hpp"
#include "widgets/dialogs/font/FontSizeWidget.hpp"
#include "widgets/dialogs/font/FontWeightWidget.hpp"
#include "widgets/dialogs/font/PreviewWidget.hpp"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino {

FontDialog::FontDialog(const QFont &startFont, QWidget *parent)
    : QDialog(parent)
    , preview(new PreviewWidget(startFont))
    , familyW(new FontFamilyWidget(startFont))
    , sizeW(new FontSizeWidget(startFont))
    , weightW(new FontWeightWidget(startFont))
{
    auto *layout = new QVBoxLayout;
    auto *choiceLayout = new QHBoxLayout;
    auto *choiceSideLayout = new QVBoxLayout;
    auto *buttonLayout = new QHBoxLayout;

    auto *applyButton = new QPushButton("Apply");
    auto *acceptButton = new QPushButton("Ok");
    auto *rejectButton = new QPushButton("Cancel");

    this->setWindowTitle("Pick Font");
    this->setLayout(layout);
    this->resize(450, 450);

    layout->addLayout(choiceLayout, 5);
    layout->addWidget(new QLabel("Preview"));
    layout->addWidget(this->preview, 1);
    layout->addLayout(buttonLayout);

    choiceLayout->addWidget(this->familyW, 5);
    choiceLayout->addLayout(choiceSideLayout, 3);

    choiceSideLayout->addWidget(this->sizeW);
    choiceSideLayout->addWidget(this->weightW);

    buttonLayout->addWidget(applyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(rejectButton);

    QObject::connect(applyButton, &QPushButton::clicked, this,
                     &FontDialog::applied);

    QObject::connect(acceptButton, &QPushButton::clicked, this,
                     &FontDialog::accept);

    QObject::connect(rejectButton, &QPushButton::clicked, this,
                     &FontDialog::reject);

    QObject::connect(this->familyW, &FontFamilyWidget::selectedChanged, this,
                     [this] {
                         this->weightW->setFamily(this->familyW->getSelected());
                         this->updatePreview();
                     });

    QObject::connect(this->weightW, &FontWeightWidget::selectedChanged, this,
                     &FontDialog::updatePreview);

    QObject::connect(this->sizeW, &FontSizeWidget::selectedChanged, this,
                     &FontDialog::updatePreview);
}

QFont FontDialog::getSelected() const
{
    return {
        this->familyW->getSelected(),
        this->sizeW->getSelected(),
        this->weightW->getSelected(),
    };
}

void FontDialog::updatePreview()
{
    this->preview->setFont(this->getSelected());
}

}  // namespace chatterino
