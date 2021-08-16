#pragma once

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/helper/LoginServer.hpp"

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtCore/QVariant>

#define LOGIN_BUTTON_START "Click to Log in"
#define TOKEN_BUTTON_START "Paste token from clipboard"
#define VALIDATING_TOKEN "Validating your token..."

namespace chatterino {

class LoginServer;

struct TokenValidationResponse {
    QString clientId;
    QString login;
    QString userId;
    std::vector<QString> scopes;
    int expiresIn;

    explicit TokenValidationResponse(QJsonObject root)
        : clientId(root.value("client_id").toString())
        , login(root.value("login").toString())
        , userId(root.value("user_id").toString())
        , expiresIn(root.value("expires_in").toInt())
    {
        for (const auto &scope : root.value("scopes").toArray())
        {
            this->scopes.emplace_back(scope.toString());
        }
    }
};

class LoginDialog : public QDialog
{
public:
    LoginDialog(QWidget *parent);

private:
    struct {
        QVBoxLayout layout;

        QLabel helpLabel;
        QLabel warningLabel;

        QHBoxLayout buttons;
        QPushButton loginButton;
        QPushButton pasteTokenButton;

        QDialogButtonBox buttonBox;
    } ui_;

    // Local server listening to login data returned from Twitch
    LoginServer *loginServer_;
    QUrl loginLink;

    void deactivateLoginButton();
    void activateLoginButton();
    void logInWithToken(QString token, std::function<void()> successCallback,
                        std::function<void()> failureCallback,
                        std::function<void()> finallyCallback);

    void hideEvent(QHideEvent *e) override;

    friend class LoginServer;
    friend class LoginBoost;
};

}  // namespace chatterino
