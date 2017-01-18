#include "textinputdialog.h"
#include <QSizePolicy>

TextInputDialog::TextInputDialog(QWidget *parent)
    : QDialog(parent)
    , vbox(this)
    , lineEdit()
    , buttonBox()
    , okButton("OK")
    , cancelButton("Cancel")
{
    this->vbox.addWidget(&this->lineEdit);
    this->vbox.addLayout(&this->buttonBox);
    this->buttonBox.addStretch(1);
    this->buttonBox.addWidget(&this->okButton);
    this->buttonBox.addWidget(&this->cancelButton);

    QObject::connect(&this->okButton, SIGNAL(clicked()), this,
                     SLOT(okButtonClicked()));
    QObject::connect(&this->cancelButton, SIGNAL(clicked()), this,
                     SLOT(cancelButtonClicked()));

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    setWindowFlags((windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
}

void
TextInputDialog::okButtonClicked()
{
    accept();
    close();
}

void
TextInputDialog::cancelButtonClicked()
{
    reject();
    close();
}
