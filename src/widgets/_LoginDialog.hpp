#pragma once

#include "widgets/basewidget.hpp"

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

namespace chatterino {
namespace widgets {

class BasicLoginWidget : public QWidget
{
public:
    BasicLoginWidget();

    struct {
        QVBoxLayout layout;
        QHBoxLayout horizontalLayout;
        QPushButton loginButton;
        QPushButton pasteCodeButton;
    } ui;
};

class AdvancedLoginWidget : public QWidget
{
public:
    AdvancedLoginWidget();

    void refreshButtons();

    struct {
        QVBoxLayout layout;

        QLabel instructionsLabel;

        QFormLayout formLayout;

        QLineEdit userIDInput;
        QLineEdit usernameInput;
        QLineEdit clientIDInput;
        QLineEdit oauthTokenInput;

        struct {
            QHBoxLayout layout;

            QPushButton addUserButton;
            QPushButton clearFieldsButton;
        } buttonUpperRow;

        struct {
            QHBoxLayout layout;

            QPushButton fillInUserIDButton;
        } buttonLowerRow;
    } ui;
};

class LoginWidget : public QDialog
{
public:
    LoginWidget();

private:
    struct {
        QVBoxLayout mainLayout;

        QTabWidget tabWidget;

        QDialogButtonBox buttonBox;

        BasicLoginWidget basic;

        AdvancedLoginWidget advanced;
    } ui;
};

}  // namespace widgets
}  // namespace chatterino
