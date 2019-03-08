#include "widgets/dialogs/TextInputDialog.hpp"
#include <QSizePolicy>

namespace chatterino
{
    TextInputDialog::TextInputDialog(QWidget* parent)
        : QDialog(parent)
        , vbox_(this)
        , okButton_("OK")
        , cancelButton_("Cancel")
    {
        this->vbox_.addWidget(&lineEdit_);
        this->vbox_.addLayout(&buttonBox_);
        this->buttonBox_.addStretch(1);
        this->buttonBox_.addWidget(&okButton_);
        this->buttonBox_.addWidget(&cancelButton_);

        QObject::connect(
            &this->okButton_, SIGNAL(clicked()), this, SLOT(okButtonClicked()));
        QObject::connect(&this->cancelButton_, SIGNAL(clicked()), this,
            SLOT(cancelButtonClicked()));

        this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        this->setWindowFlags(
            (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
            Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    }

    QString TextInputDialog::getText() const
    {
        return this->lineEdit_.text();
    }

    void TextInputDialog::setText(const QString& text)
    {
        this->lineEdit_.setText(text);
    }

    void TextInputDialog::okButtonClicked()
    {
        this->accept();
        this->close();
    }

    void TextInputDialog::cancelButtonClicked()
    {
        this->reject();
        this->close();
    }

    void TextInputDialog::highlightText()
    {
        this->lineEdit_.selectAll();
    }
}  // namespace chatterino
