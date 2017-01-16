#include "textinputdialog.h"
#include <QSizePolicy>

TextInputDialog::TextInputDialog(QWidget *parent)
    : QDialog(parent)
    , m_vbox(this)
    , m_lineEdit()
    , m_buttonBox()
    , m_okButton("OK")
    , m_cancelButton("Cancel")
{
    m_vbox.addWidget(&m_lineEdit);
    m_vbox.addLayout(&m_buttonBox);
    m_buttonBox.addStretch(1);
    m_buttonBox.addWidget(&m_okButton);
    m_buttonBox.addWidget(&m_cancelButton);

    QObject::connect(&m_okButton, &m_okButton.clicked, this, &okButtonClicked);
    QObject::connect(&m_cancelButton, &m_cancelButton.clicked, this,
                     &cancelButtonClicked);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    setWindowFlags((windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
}

void
TextInputDialog::okButtonClicked()
{
    accept();
}

void
TextInputDialog::cancelButtonClicked()
{
    reject();
}
