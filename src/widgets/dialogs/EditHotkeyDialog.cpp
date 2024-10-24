#include "widgets/dialogs/EditHotkeyDialog.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/ActionNames.hpp"
#include "controllers/hotkeys/Hotkey.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/hotkeys/HotkeyHelpers.hpp"
#include "ui_EditHotkeyDialog.h"

namespace chatterino {

EditHotkeyDialog::EditHotkeyDialog(const std::shared_ptr<Hotkey> hotkey,
                                   bool isAdd, QWidget *parent)
    : QDialog(parent, Qt::WindowStaysOnTopHint)
    , ui_(new Ui::EditHotkeyDialog)
    , data_(hotkey)
{
    this->ui_->setupUi(this);
    this->setStyleSheet(R"(QToolTip {
    padding: 2px;
    background-color: #333333;
    border: 1px solid #545454;
    color: white;
})");
    this->ui_->easyArgsPicker->setVisible(false);
    this->ui_->easyArgsLabel->setVisible(false);
    // dynamically add category names to the category picker
    for (const auto &[_, hotkeyCategory] : hotkeyCategories())
    {
        this->ui_->categoryPicker->addItem(hotkeyCategory.displayName,
                                           hotkeyCategory.name);
    }

    this->ui_->warningLabel->hide();

    if (hotkey)
    {
        this->setFromHotkey(hotkey);
    }
    else
    {
        // adding a new hotkey
        this->setWindowTitle("Add hotkey");
        this->ui_->categoryPicker->setCurrentIndex(
            size_t(HotkeyCategory::SplitInput));
        this->ui_->argumentsEdit->setPlainText("");
    }
}
void EditHotkeyDialog::setFromHotkey(std::shared_ptr<Hotkey> hotkey)
{
    if (!hotkey->validAction())
    {
        this->showEditError("Invalid action, make sure you select the "
                            "correct action before saving.");
    }

    // editing a hotkey

    // update pickers/input boxes to values from Hotkey object
    this->ui_->categoryPicker->setCurrentIndex(size_t(hotkey->category()));
    this->ui_->keyComboEdit->setKeySequence(
        QKeySequence::fromString(hotkey->keySequence().toString()));
    this->ui_->nameEdit->setText(hotkey->name());

    auto def = findHotkeyActionDefinition(hotkey->category(), hotkey->action());
    if (def.has_value() && !def->possibleArguments.empty())
    {
        qCDebug(chatterinoHotkeys) << "Enabled easy picker and arg edit "
                                      "because we have arguments from hotkey";
        this->ui_->easyArgsLabel->setVisible(true);
        this->ui_->easyArgsPicker->setVisible(true);

        this->ui_->argumentsEdit->setVisible(false);
        this->ui_->argumentsLabel->setVisible(false);
        this->ui_->argumentsDescription->setVisible(false);

        this->ui_->easyArgsPicker->clear();
        this->ui_->easyArgsLabel->setText(def->argumentsPrompt);
        this->ui_->easyArgsLabel->setToolTip(def->argumentsPromptHover);
        int matchIdx = -1;
        for (size_t i = 0; i < def->possibleArguments.size(); i++)
        {
            const auto &[displayText, argData] = def->possibleArguments.at(i);
            this->ui_->easyArgsPicker->addItem(displayText);

            // check if matches
            if (argData.size() != hotkey->arguments().size())
            {
                continue;
            }
            bool matches = true;
            for (size_t j = 0; j < argData.size(); j++)
            {
                if (argData.at(j) != hotkey->arguments().at(j))
                {
                    matches = false;
                    break;
                }
            }
            if (matches)
            {
                matchIdx = static_cast<int>(i);
            }
        }
        if (matchIdx != -1)
        {
            this->ui_->easyArgsPicker->setCurrentIndex(matchIdx);
            return;
        }

        qCDebug(chatterinoHotkeys)
            << "Did not match hotkey arguments for " << hotkey->toString()
            << "using text edit instead of easy picker";
        this->showEditError("Arguments do not match what's expected. The "
                            "argument picker is not available.");
        this->ui_->easyArgsLabel->setVisible(false);
        this->ui_->easyArgsPicker->setVisible(false);

        this->ui_->argumentsEdit->setVisible(true);
        this->ui_->argumentsLabel->setVisible(true);
        this->ui_->argumentsDescription->setVisible(true);
    }
    // update arguments
    QString argsText;
    bool first = true;
    for (const auto &arg : hotkey->arguments())
    {
        if (!first)
        {
            argsText += '\n';
        }

        argsText += arg;

        first = false;
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

void EditHotkeyDialog::afterEdit()
{
    auto arguments =
        parseHotkeyArguments(this->ui_->argumentsEdit->toPlainText());

    auto category = getApp()->getHotkeys()->hotkeyCategoryFromName(
        this->ui_->categoryPicker->currentData().toString());
    if (!category)
    {
        this->showEditError("Invalid Hotkey Category.");

        return;
    }
    QString nameText = this->ui_->nameEdit->text();

    // check if another hotkey with this name exists, accounts for editing a hotkey
    bool isEditing = bool(this->data_);
    if (getApp()->getHotkeys()->getHotkeyByName(nameText))
    {
        // A hotkey with this name already exists
        if (isEditing && this->data()->name() == nameText)
        {
            // The hotkey that already exists is the one we are editing
        }
        else
        {
            // The user is either creating a hotkey with a name that already exists, or
            // the user is editing an already-existing hotkey and changing its name to a hotkey that already exists
            this->showEditError("Hotkey with this name already exists.");
            return;
        }
    }
    if (nameText.isEmpty())
    {
        this->showEditError("Hotkey name is missing");
        return;
    }
    if (this->ui_->keyComboEdit->keySequence().count() == 0)
    {
        this->showEditError("Key Sequence is missing");
        return;
    }
    if (this->ui_->actionPicker->currentText().isEmpty())
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
            "Warning: using keybindings without modifiers can lead to not "
            "being\nable to use the key for the normal purpose.\nPress the "
            "submit button again to do it anyway.");
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

    auto def = findHotkeyActionDefinition(*category, action);
    if (def.has_value() && this->ui_->easyArgsPicker->isVisible())
    {
        arguments =
            def->possibleArguments.at(this->ui_->easyArgsPicker->currentIndex())
                .second;
    }

    auto hotkey = std::make_shared<Hotkey>(
        *category, this->ui_->keyComboEdit->keySequence(), action, arguments,
        nameText);
    auto keyComboWasEdited =
        this->data() &&
        this->ui_->keyComboEdit->keySequence() != this->data()->keySequence();
    auto nameWasEdited = this->data() && nameText != this->data()->name();

    if (isEditing)
    {
        if (keyComboWasEdited || nameWasEdited)
        {
            if (getApp()->getHotkeys()->isDuplicate(hotkey,
                                                    this->data()->name()))
            {
                this->showEditError(
                    "Keybinding needs to be unique in the category.");
                return;
            }
        }
    }
    else
    {
        if (getApp()->getHotkeys()->isDuplicate(hotkey, QString()))
        {
            this->showEditError(
                "Keybinding needs to be unique in the category.");
            return;
        }
    }

    this->data_ = hotkey;
    this->accept();
}

void EditHotkeyDialog::updatePossibleActions()
{
    const auto &hotkeys = getApp()->getHotkeys();
    auto category = hotkeys->hotkeyCategoryFromName(
        this->ui_->categoryPicker->currentData().toString());
    if (!category)
    {
        this->showEditError("Invalid Hotkey Category.");

        return;
    }
    auto currentText = this->ui_->actionPicker->currentData().toString();
    if (this->data_ &&
        (currentText.isEmpty() || this->data_->category() == category))
    {
        // is editing
        currentText = this->data_->action();
    }
    this->ui_->actionPicker->clear();
    qCDebug(chatterinoHotkeys)
        << "update possible actions for" << (int)*category << currentText;
    auto actions = actionNames.find(*category);
    if (actions != actionNames.end())
    {
        int indexToSet = -1;
        for (const auto &action : actions->second)
        {
            this->ui_->actionPicker->addItem(action.second.displayName,
                                             action.first);
            if (action.first == currentText)
            {
                // update action raw name to display name
                indexToSet = this->ui_->actionPicker->model()->rowCount() - 1;
            }
        }
        if (indexToSet != -1)
        {
            this->ui_->actionPicker->setCurrentIndex(indexToSet);
        }
    }
    else
    {
        qCDebug(chatterinoHotkeys) << "key missing!!!!";
    }
}

void EditHotkeyDialog::updateArgumentsInput()
{
    auto currentText = this->ui_->actionPicker->currentData().toString();
    if (currentText.isEmpty())
    {
        this->ui_->argumentsEdit->setEnabled(true);
        return;
    }
    const auto &hotkeys = getApp()->getHotkeys();
    auto category = hotkeys->hotkeyCategoryFromName(
        this->ui_->categoryPicker->currentData().toString());
    if (!category)
    {
        this->showEditError("Invalid Hotkey category.");

        return;
    }
    auto allActions = actionNames.find(*category);
    if (allActions != actionNames.end())
    {
        const auto &actionsMap = allActions->second;
        auto definition = actionsMap.find(currentText);
        if (definition == actionsMap.end())
        {
            auto text = QString("Newline separated arguments for the action\n"
                                " - Unable to find action named \"%1\"")
                            .arg(currentText);
            this->ui_->argumentsEdit->setPlaceholderText(text);
            return;
        }
        const ActionDefinition &def = definition->second;

        if (def.maxCountArguments == 0)
        {
            qCDebug(chatterinoHotkeys) << "Disabled easy picker and arg edit "
                                          "because we don't have any arguments";
            this->ui_->argumentsLabel->setVisible(false);
            this->ui_->argumentsDescription->setVisible(false);
            this->ui_->argumentsEdit->setVisible(false);

            this->ui_->easyArgsLabel->setVisible(false);
            this->ui_->easyArgsPicker->setVisible(false);
            return;
        }
        if (!def.argumentDescription.isEmpty())
        {
            this->ui_->argumentsDescription->setVisible(true);
            this->ui_->argumentsDescription->setText(def.argumentDescription);
        }
        else
        {
            this->ui_->argumentsDescription->setVisible(false);
        }

        QString text = "Arguments wrapped in <> are required.";
        if (def.maxCountArguments != def.minCountArguments)
        {
            text += QString("\nArguments wrapped in [] are optional.");
        }

        text += "\nArguments are separated by a newline.";

        this->ui_->argumentsEdit->setEnabled(true);
        this->ui_->argumentsEdit->setPlaceholderText(text);

        this->ui_->argumentsLabel->setVisible(true);
        this->ui_->argumentsDescription->setVisible(true);
        this->ui_->argumentsEdit->setVisible(true);

        // update easy picker
        if (def.possibleArguments.empty())
        {
            qCDebug(chatterinoHotkeys)
                << "Disabled easy picker because we have possible arguments";
            this->ui_->easyArgsPicker->setVisible(false);
            this->ui_->easyArgsLabel->setVisible(false);
            return;
        }
        qCDebug(chatterinoHotkeys)
            << "Enabled easy picker because we have possible arguments";
        this->ui_->easyArgsPicker->setVisible(true);
        this->ui_->easyArgsLabel->setVisible(true);

        this->ui_->argumentsLabel->setVisible(false);
        this->ui_->argumentsEdit->setVisible(false);
        this->ui_->argumentsDescription->setVisible(false);

        this->ui_->easyArgsPicker->clear();
        for (const auto &[displayText, _] : def.possibleArguments)
        {
            this->ui_->easyArgsPicker->addItem(displayText);
        }
        this->ui_->easyArgsPicker->setCurrentIndex(0);
        this->ui_->easyArgsLabel->setText(def.argumentsPrompt);
        this->ui_->easyArgsLabel->setToolTip(def.argumentsPromptHover);
    }
}

void EditHotkeyDialog::showEditError(QString errorText)
{
    this->ui_->warningLabel->setText(errorText);
    this->ui_->warningLabel->show();
}

}  // namespace chatterino
