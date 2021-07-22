#pragma once

#include "widgets/BaseWidget.hpp"

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVBoxLayout>
#include <QtCore/QVariant>
#include <QtHttpServer/QHttpServer>

namespace chatterino {

class BasicLoginWidget : public QWidget
{
public:
    BasicLoginWidget();

    struct {
        QVBoxLayout layout;
        QHBoxLayout horizontalLayout;
        QPushButton loginButton;
        QPushButton pasteCodeButton;
        QLabel unableToOpenBrowserHelper;
    } ui_;

    void closeHttpServer();

private:
    // Local server listening to login data
    const QHostAddress serverAddress{QHostAddress::LocalHost};
    static const int serverPort = 52107;
    QHttpServer *httpServer_;
    QTcpServer *tcpServer_;
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
    } ui_;
};

class LoginWidget : public QDialog
{
public:
    LoginWidget(QWidget *parent);

private:
    struct {
        QVBoxLayout mainLayout;

        QTabWidget tabWidget;

        QDialogButtonBox buttonBox;

        BasicLoginWidget basic;

        AdvancedLoginWidget advanced;
    } ui_;

    void hideEvent(QHideEvent *e) override;
};

}  // namespace chatterino
