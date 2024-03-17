#include "widgets/dialogs/HighlightConfigureDialog.hpp"

#include "widgets/dialogs/ui_HighlightConfigureDialog.h"

namespace chatterino {

HighlightConfigureDialog::HighlightConfigureDialog(HighlightPhrase phrase,
                                                   QWidget *parent)

    : QDialog(parent, Qt::WindowStaysOnTopHint)
    , ui_(new Ui::HighlightConfigureDialog)
{
    this->ui_->setupUi(this);

    this->ui_->patternLineEdit->setText(phrase.getPattern());
}

HighlightConfigureDialog::~HighlightConfigureDialog()
{
    delete ui_;
}

}  // namespace chatterino
