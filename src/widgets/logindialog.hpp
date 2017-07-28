#pragma once

#include "widgets/basewidget.hpp"

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace chatterino {
namespace widgets {

class LoginWidget : public QDialog
{
public:
    LoginWidget();

private:
    struct {
        QVBoxLayout mainLayout;

        QVBoxLayout verticalLayout;
        QHBoxLayout horizontalLayout;
        QPushButton loginButton;
        QPushButton pasteCodeButton;
        QDialogButtonBox buttonBox;
    } ui;
};

}  // namespace widgets
}  // namespace chatterino
