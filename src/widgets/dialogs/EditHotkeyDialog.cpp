#include "EditHotkeyDialog.hpp"
#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/ActionNames.hpp"
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
    // dynamically add scope names to the scope picker
    for (const auto pair : app->hotkeys->hotkeyScopeNames)
    {
        this->ui_->scopePicker->addItem(
            app->hotkeys->hotkeyScopeDisplayNames.find(pair.first)->second,
            pair.second);
    }

    if (hotkey)
    {
        // editting a hotkey

        // update pickers/input boxes to values from Hotkey object
        this->ui_->scopePicker->setCurrentIndex(size_t(hotkey->scope()));
        this->ui_->keyComboEdit->setKeySequence(
            QKeySequence::fromString(hotkey->keySequence().toString()));
        this->ui_->nameEdit->setText(hotkey->name());
        this->ui_->actionPicker->setCurrentText(hotkey->action());
        this->updatePossibleActions();  // make sure the action names are available

        // update arguments
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
        // adding a new hotkey
        this->setWindowTitle("Add hotkey");
        this->ui_->scopePicker->setCurrentIndex(
            size_t(HotkeyScope::SplitInput));
        this->ui_->argumentsEdit->setPlainText("");
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

    auto argTemp = this->ui_->argumentsEdit->toPlainText().split("\n");
    // if the arguments input is empty then make sure arguments are empty.
    if (!(argTemp.size() == 1 && argTemp.at(0) == ""))
    {
        for (const auto arg : argTemp)
        {
            arguments.push_back(arg);
        }
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
    if (nameText == "")
    {
        this->showEditError("Hotkey name is missing");
        return;
    }
    if (this->ui_->keyComboEdit->keySequence().count() == 0)
    {
        this->showEditError("Key Sequence is missing");
        return;
    }
    if (this->ui_->actionPicker->currentText() == "")
    {
        this->showEditError("Action name cannot be empty");
        return;
    }

    auto firstKeyInt = this->ui_->keyComboEdit->keySequence()[0];
    bool hasModifier = ((firstKeyInt & Qt::CTRL) == Qt::CTRL) ||
                       ((firstKeyInt & Qt::ALT) == Qt::ALT) ||
                       ((firstKeyInt & Qt::META) == Qt::META);
    bool isKeyExcempt = ((firstKeyInt & Qt::Key_Escape) == Qt::Key_Escape) ||
                        ((firstKeyInt & Qt::Key_Enter) == Qt::Key_Enter) ||
                        ((firstKeyInt & Qt::Key_Return) == Qt::Key_Return);

    if (!isKeyExcempt && !hasModifier && !this->shownSingleKeyWarning)
    {
        this->showEditError(
            "Warning: hotkeys without modifiers can lead to not being "
            "\nable to use the key for the normal purpose.\nSubmit again to do "
            "it anyway.");
        this->shownSingleKeyWarning = true;
        return;
    }

    // use raw name from item data if possible, otherwise fallback to what the user has entered.
    auto actionTemp = this->ui_->actionPicker->currentData();
    QString action = this->ui_->actionPicker->currentText();
    if (actionTemp.isValid())
    {
        action = actionTemp.toString();
    }

    auto hotkey =
        std::make_shared<Hotkey>(*scope, this->ui_->keyComboEdit->keySequence(),
                                 action, arguments, nameText);
    if (getApp()->hotkeys->isDuplicate(hotkey))
    {
        this->showEditError("Key combo needs to be unique in the scope.");
        return;
    }

    this->data_ = hotkey;
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
    auto actions = actionNames.find(*scope);
    if (actions != actionNames.end())
    {
        const auto currentText = this->ui_->actionPicker->currentText();
        int index = 0;
        for (const auto action : actions->second)
        {
            this->ui_->actionPicker->addItem(action.second.displayName,
                                             action.first);
            if (action.first == currentText)
            {
                // update action raw name to display name
                this->ui_->actionPicker->setCurrentIndex(index);
            }
            index++;
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
