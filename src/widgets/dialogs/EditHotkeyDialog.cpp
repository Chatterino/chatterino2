#include "EditHotkeyDialog.hpp"
#include "Application.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "ui_EditHotkeyDialog.h"

namespace chatterino {
EditHotkeyDialog::EditHotkeyDialog(const std::shared_ptr<Hotkey> hotkey,
                                   bool isAdd, QWidget *parent)
    : QDialog(parent, Qt::WindowStaysOnTopHint)
    , ui_(new Ui::EditHotkeyDialog)
    , data_(hotkey)
{
    this->ui_->setupUi(this);
    if (hotkey)
    {
        this->ui_->scopeEdit->setText(
            HotkeyController::hotkeyScopeToName(hotkey->scope()));
        this->ui_->actionEdit->setText(hotkey->action());
        this->ui_->keyComboEdit->setKeySequence(
            QKeySequence::fromString(hotkey->keySequence().toString()));
        this->ui_->nameEdit->setText(hotkey->name());

        bool isFirst = true;
        QString argsText;
        for (const auto arg : hotkey->arguments())
        {
            if (!isFirst)
            {
                argsText += '\n';
            }
            argsText += arg;
            isFirst = false;
        }
        this->ui_->argumentsEdit->setPlainText(argsText);
    }
    this->ui_->warningLabel->hide();
}

EditHotkeyDialog::~EditHotkeyDialog()
{
    delete this->ui_;
}

std::shared_ptr<Hotkey> EditHotkeyDialog::data()
{
    return this->data_;
}

void EditHotkeyDialog::afterEdit()
{
    std::vector<QString> arguments;
    for (const auto arg : this->ui_->argumentsEdit->toPlainText().split("\n"))
    {
        arguments.push_back(arg);
    }
    auto scope =
        HotkeyController::hotkeyScopeFromName(this->ui_->scopeEdit->text());
    if (!scope)
    {
        this->showEditError("Invalid Hotkey Scope.");

        return;
    }
    QString nameText = this->ui_->nameEdit->text();

    // check if another hotkey with this name exists, accounts for editing a hotkey
    if (getApp()->hotkeys->getHotkeyByName(nameText) &&
        !(this->data_ && this->data_->name() == nameText))
    {
        this->showEditError("Hotkey with this name already exists.");
        return;
    }
    this->data_ = std::make_shared<Hotkey>(
        *scope, this->ui_->keyComboEdit->keySequence(),
        this->ui_->actionEdit->text(), arguments, nameText);
    this->accept();
}
void EditHotkeyDialog::showEditError(QString errorText)
{
    this->ui_->warningLabel->setText(errorText);
    this->ui_->warningLabel->show();
}
}  // namespace chatterino
