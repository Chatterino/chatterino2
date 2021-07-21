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

namespace chatterino {

class LoginServer : public QTcpServer
{
public:
    static constexpr int chatterinoPort = 52107;

    explicit LoginServer(QObject *parent = {});
    QTcpServer *getServer();
    //    void incomingConnection(qintptr handle) override;
public slots:
    void slotNewConnection();
    void slotServerRead();
    void slotBytesWritten();
    void slotClientDisconnected();

    //public slots:
    //    void newConnection();

private:
    QTcpServer *server_;
    QTcpSocket *socket_;
};

class BasicLoginWidget : public QWidget
{
public:
    BasicLoginWidget();
    ~BasicLoginWidget();

    struct {
        QVBoxLayout layout;
        QHBoxLayout horizontalLayout;
        QPushButton loginButton;
        QPushButton pasteCodeButton;
        QLabel unableToOpenBrowserHelper;
    } ui_;

    //private:
    // Local server listening to login data
    LoginServer *loginServer_;
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
