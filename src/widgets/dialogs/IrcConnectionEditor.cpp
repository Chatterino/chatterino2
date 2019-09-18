#include "IrcConnectionEditor.hpp"
#include "ui_IrcConnectionEditor.h"

namespace chatterino {

IrcConnectionEditor::IrcConnectionEditor(const IrcServerData &data, bool isAdd,
                                         QWidget *parent)

    : QDialog(parent, Qt::WindowStaysOnTopHint)
    , ui_(new Ui::IrcConnectionEditor)
    , data_(data)
{
    this->ui_->setupUi(this);

    this->setWindowTitle(QString(isAdd ? "Add " : "Edit ") + "Irc Connection");

    QObject::connect(this->ui_->userNameLineEdit, &QLineEdit::textChanged, this,
                     [this](const QString &text) {
                         this->ui_->nickNameLineEdit->setPlaceholderText(text);
                         this->ui_->realNameLineEdit->setPlaceholderText(text);
                     });

    this->ui_->serverLineEdit->setText(data.host);
    this->ui_->portSpinBox->setValue(data.port);
    this->ui_->securityCheckBox->setChecked(data.ssl);
    this->ui_->userNameLineEdit->setText(data.user);
    this->ui_->nickNameLineEdit->setText(data.nick);
    this->ui_->realNameLineEdit->setText(data.real);
    this->ui_->connectCommandsEditor->setPlainText(
        data.connectCommands.join('\n'));

    data.getPassword(this, [this](const QString &password) {
        this->ui_->passwordLineEdit->setText(password);
    });

    this->ui_->loginMethodComboBox->setCurrentIndex([&] {
        switch (data.authType)
        {
            case IrcAuthType::Custom:
                return 1;
            case IrcAuthType::Pass:
                return 2;
            case IrcAuthType::Sasl:
                return 3;
            default:
                return 0;
        }
    }());

    QObject::connect(this->ui_->loginMethodComboBox,
                     qOverload<int>(&QComboBox::currentIndexChanged), this,
                     [this](int index) {
                         if (index == 1)  // Custom
                         {
                             this->ui_->connectCommandsEditor->setFocus();
                         }
                     });

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    this->ui_->connectCommandsEditor->setFont(font);
}

IrcConnectionEditor::~IrcConnectionEditor()
{
    delete ui_;
}

IrcServerData IrcConnectionEditor::data()
{
    auto data = this->data_;
    data.host = this->ui_->serverLineEdit->text();
    data.port = this->ui_->portSpinBox->value();
    data.ssl = this->ui_->securityCheckBox->isChecked();
    data.user = this->ui_->userNameLineEdit->text();
    data.nick = this->ui_->nickNameLineEdit->text();
    data.real = this->ui_->realNameLineEdit->text();
    data.connectCommands =
        this->ui_->connectCommandsEditor->toPlainText().split('\n');
    data.setPassword(this->ui_->passwordLineEdit->text());
    data.authType = [this] {
        switch (this->ui_->loginMethodComboBox->currentIndex())
        {
            case 1:
                return IrcAuthType::Custom;
            case 2:
                return IrcAuthType::Pass;
            case 3:
                return IrcAuthType::Sasl;
            default:
                return IrcAuthType::Anonymous;
        }
    }();
    return data;
}

}  // namespace chatterino
