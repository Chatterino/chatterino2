#include "widgets/textinputdialog.hpp"
#include <QSizePolicy>

namespace chatterino {
namespace widgets {

TextInputDialog::TextInputDialog(QWidget *parent)
    : QDialog(parent)
    , _vbox(this)
    , _okButton("OK")
    , _cancelButton("Cancel")
{
    _vbox.addWidget(&_lineEdit);
    _vbox.addLayout(&_buttonBox);
    _buttonBox.addStretch(1);
    _buttonBox.addWidget(&_okButton);
    _buttonBox.addWidget(&_cancelButton);

    QObject::connect(&_okButton, SIGNAL(clicked()), this, SLOT(okButtonClicked()));
    QObject::connect(&_cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    setWindowFlags((windowFlags() & ~(Qt::WindowContextHelpButtonHint)) | Qt::Dialog |
                   Qt::MSWindowsFixedSizeDialogHint);
}

void TextInputDialog::okButtonClicked()
{
    accept();
    close();
}

void TextInputDialog::cancelButtonClicked()
{
    reject();
    close();
}

}  // namespace widgets
}  // namespace chatterino
