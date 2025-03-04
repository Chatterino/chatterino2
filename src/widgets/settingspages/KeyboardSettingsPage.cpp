#include "widgets/settingspages/KeyboardSettingsPage.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/Hotkey.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/hotkeys/HotkeyModel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/dialogs/EditHotkeyDialog.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QFormLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QMessageBox>
#include <QTableView>

#include <array>

namespace {

using namespace chatterino;

void tableCellClicked(const QModelIndex &clicked, EditableModelView *view,
                      HotkeyModel *model)
{
    auto hotkey = getApp()->getHotkeys()->getHotkeyByName(
        clicked.siblingAtColumn(0).data(Qt::EditRole).toString());
    if (!hotkey)
    {
        return;  // clicked on header or invalid hotkey
    }
    EditHotkeyDialog dialog(hotkey);
    bool wasAccepted = dialog.exec() == 1;

    if (wasAccepted)
    {
        auto newHotkey = dialog.data();
        getApp()->getHotkeys()->replaceHotkey(hotkey->name(), newHotkey);
        getApp()->getHotkeys()->save();
    }
}

}  // namespace

namespace chatterino {

KeyboardSettingsPage::KeyboardSettingsPage()
{
    LayoutCreator<KeyboardSettingsPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>();

    auto *model = getApp()->getHotkeys()->createModel(nullptr);
    EditableModelView *view =
        layout.emplace<EditableModelView>(model).getElement();
    this->view_ = view;

    view->setTitles({"Hotkey name", "Keybinding"});
    view->getTableView()->horizontalHeader()->setVisible(true);
    view->getTableView()->horizontalHeader()->setStretchLastSection(false);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    // We can safely ignore this signal connection since we own the view
    std::ignore = view->addButtonPressed.connect([] {
        EditHotkeyDialog dialog(nullptr);
        bool wasAccepted = dialog.exec() == 1;

        if (wasAccepted)
        {
            auto newHotkey = dialog.data();
            getApp()->getHotkeys()->hotkeys_.append(newHotkey);
            getApp()->getHotkeys()->save();
        }
    });

    QObject::connect(view_->getTableView(), &QTableView::doubleClicked,
                     [view, model](const QModelIndex &clicked) {
                         tableCellClicked(clicked, view, model);
                     });

    auto *keySequenceInput = new QKeySequenceEdit(this);

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    keySequenceInput->setClearButtonEnabled(true);
#endif
    auto *searchText = new QLabel("Search keybind:", this);

    QObject::connect(keySequenceInput, &QKeySequenceEdit::keySequenceChanged,
                     [view](const QKeySequence &keySequence) {
                         view->filterSearchResultsHotkey(keySequence);
                     });
    view->addCustomButton(searchText);
    view->addCustomButton(keySequenceInput);

    auto *resetEverything = new QPushButton("Reset to defaults");
    QObject::connect(resetEverything, &QPushButton::clicked, [this]() {
        auto reply = QMessageBox::question(
            this, "Reset hotkeys",
            "Are you sure you want to reset hotkeys to defaults?",
            QMessageBox::Yes | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
        {
            getApp()->getHotkeys()->resetToDefaults();
        }
    });
    view->addCustomButton(resetEverything);

    // We only check this once since a user *should* not have the ability to create a new hotkey with a deprecated or removed action
    // However, we also don't update this after the user has deleted a hotkey. This is a big lift that should probably be solved on the model level rather
    // than individually here. Same goes for marking specific rows as deprecated/removed
    const auto &removedOrDeprecatedHotkeys =
        getApp()->getHotkeys()->removedOrDeprecatedHotkeys();

    if (!removedOrDeprecatedHotkeys.empty())
    {
        QString warningMessage =
            "Some of your hotkeys use deprecated actions and will not "
            "work as expected: ";

        bool first = true;
        for (const auto &hotkeyName : removedOrDeprecatedHotkeys)
        {
            if (!first)
            {
                warningMessage.append(',');
            }
            warningMessage.append(' ');
            warningMessage.append('"');
            warningMessage.append(hotkeyName);
            warningMessage.append('"');

            first = false;
        }
        warningMessage.append('.');
        auto deprecatedWarning = layout.emplace<QLabel>(warningMessage);
        deprecatedWarning->setStyleSheet("color: yellow");
        deprecatedWarning->setWordWrap(true);
        auto deprecatedWarning2 = layout.emplace<QLabel>(
            "You can ignore this warning after you have removed or edited the "
            "above-mentioned hotkeys.");
        deprecatedWarning2->setStyleSheet("color: yellow");
    }
}

bool KeyboardSettingsPage::filterElements(const QString &query)
{
    std::array fields{0};

    return this->view_->filterSearchResults(query, fields);
}

}  // namespace chatterino
