#include "EditHotkeyDialog.hpp"
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
    this->ui_->scopeEdit->setText(
        HotkeyController::hotkeyScopeToName(hotkey->scope()));
    this->ui_->actionEdit->setText(hotkey->action());
    this->ui_->keyComboEdit->setText(hotkey->keySequence().toString());
    this->ui_->nameEdit->setText(hotkey->name());

    bool isFirst = false;
    QString argsText;
    for (const auto arg : hotkey->arguments())
    {
        if (!isFirst)
        {
            argsText += '\n';
        }
        argsText += arg;
    }
    this->ui_->argumentsEdit->setPlainText(argsText);
}

EditHotkeyDialog::~EditHotkeyDialog()
{
    delete this->ui_;
}

std::shared_ptr<Hotkey> EditHotkeyDialog::data()
{
    return this->data_;
}
std::shared_ptr<Hotkey> EditHotkeyDialog::afterEdit()

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
        return this->data_;
    }
    return std::make_shared<Hotkey>(
        *scope, QKeySequence::fromString(this->ui_->keyComboEdit->text()),
        this->ui_->actionEdit->text(), arguments, this->ui_->nameEdit->text());
}
}  // namespace chatterino
