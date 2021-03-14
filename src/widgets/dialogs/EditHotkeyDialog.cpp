#include "EditHotkeyDialog.hpp"
#include "Application.hpp"
#include "common/QLogging.hpp"
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
    const auto app = getApp();
    for (unsigned long i = 0; i < app->hotkeys->hotkeyScopeNames.size(); i++)
    {
        this->ui_->scopePicker->addItem(
            app->hotkeys->hotkeyScopeDisplayNames.at(i),
            app->hotkeys->hotkeyScopeNames.at(i));
    }
    if (hotkey)
    {
        this->ui_->scopePicker->setCurrentIndex((size_t)hotkey->scope());
        this->ui_->actionPicker->setCurrentText(hotkey->action());
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
    else
    {
        this->setWindowTitle("Add hotkey");
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
    auto scope = getApp()->hotkeys->hotkeyScopeFromName(
        this->ui_->scopePicker->currentData().toString());
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

    auto firstKeyInt = this->ui_->keyComboEdit->keySequence()[0];
    bool hasModifier = ((firstKeyInt & Qt::CTRL) == Qt::CTRL) ||
                       ((firstKeyInt & Qt::ALT) == Qt::ALT) ||
                       ((firstKeyInt & Qt::META) == Qt::META);
    if (!hasModifier && !this->shownSingleKeyWarning)
    {
        this->showEditError(
            "Warning: hotkeys without modifiers can lead to not being "
            "\nable to use the key for the normal purpose.\nSubmit again to do "
            "it anyway.");
        this->shownSingleKeyWarning = true;
        return;
    }
    this->data_ = std::make_shared<Hotkey>(
        *scope, this->ui_->keyComboEdit->keySequence(),
        this->ui_->actionPicker->currentText(), arguments, nameText);
    this->accept();
}

void EditHotkeyDialog::updatePossibleActions()
{
    const auto &hotkeys = getApp()->hotkeys;
    this->ui_->actionPicker->clear();
    auto scope = hotkeys->hotkeyScopeFromName(
        this->ui_->scopePicker->currentData().toString());
    if (!scope)
    {
        this->showEditError("Invalid Hotkey Scope.");

        return;
    }
    qCDebug(chatterinoHotkeys) << "update possible actions for" << (int)*scope;
    auto actions = hotkeys->savedActions.find(*scope);
    if (actions != hotkeys->savedActions.end())
    {
        for (const auto action : actions->second)
        {
            this->ui_->actionPicker->addItem(action);
        }
        qCDebug(chatterinoHotkeys) << actions->second.size();
    }
    else
    {
        qCDebug(chatterinoHotkeys) << "key missing!!!!";
    }
}

void EditHotkeyDialog::showEditError(QString errorText)
{
    this->ui_->warningLabel->setText(errorText);
    this->ui_->warningLabel->show();
}
}  // namespace chatterino
