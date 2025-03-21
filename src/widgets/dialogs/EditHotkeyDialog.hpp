#pragma once

#include <QDialog>

#include <memory>

namespace Ui {

class EditHotkeyDialog;

}  // namespace Ui

namespace chatterino {

class Hotkey;

class EditHotkeyDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit EditHotkeyDialog(const std::shared_ptr<Hotkey> data,
                              bool isAdd = false, QWidget *parent = nullptr);
    ~EditHotkeyDialog() final;

    std::shared_ptr<Hotkey> data();

protected Q_SLOTS:
    /**
     * @brief validates the hotkey
     *
     * fired by the ok button
     **/
    void afterEdit();

    /**
     * @brief updates the list of actions based on the category
     *
     * fired by the category picker changing
     **/
    void updatePossibleActions();

    /**
     * @brief updates the arguments description and input visibility
     *
     * fired by the action picker changing
     **/
    void updateArgumentsInput();

private:
    void showEditError(QString errorText);
    void setFromHotkey(std::shared_ptr<Hotkey> hotkey);

    Ui::EditHotkeyDialog *ui_;
    std::shared_ptr<Hotkey> data_;

    bool shownSingleKeyWarning = false;
};

}  // namespace chatterino
