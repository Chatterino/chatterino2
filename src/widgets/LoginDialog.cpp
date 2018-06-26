#include "widgets/LoginDialog.hpp"
#include "Common.hpp"
#include "util/UrlFetch.hpp"
#ifdef USEWINSDK
#include <Windows.h>
#endif

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <pajlada/settings/setting.hpp>

namespace chatterino {
namespace widgets {

namespace {

void LogInWithCredentials(const std::string &userID, const std::string &username,
                          const std::string &clientID, const std::string &oauthToken)
{
    QStringList errors;

    if (userID.empty()) {
        errors.append("Missing user ID");
    }
    if (username.empty()) {
        errors.append("Missing username");
    }
    if (clientID.empty()) {
        errors.append("Missing Client ID");
    }
    if (oauthToken.empty()) {
        errors.append("Missing OAuth Token");
    }

    if (errors.length() > 0) {
        QMessageBox messageBox;
        messageBox.setIcon(QMessageBox::Critical);
        messageBox.setText(errors.join("<br />"));
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        return;
    }

    //    QMessageBox messageBox;
    //    messageBox.setIcon(QMessageBox::Information);
    //    messageBox.setText("Successfully logged in with user <b>" + qS(username) + "</b>!");
    pajlada::Settings::Setting<std::string>::set("/accounts/uid" + userID + "/username", username);
    pajlada::Settings::Setting<std::string>::set("/accounts/uid" + userID + "/userID", userID);
    pajlada::Settings::Setting<std::string>::set("/accounts/uid" + userID + "/clientID", clientID);
    pajlada::Settings::Setting<std::string>::set("/accounts/uid" + userID + "/oauthToken",
                                                 oauthToken);

    getApp()->accounts->twitch.reloadUsers();

    //    messageBox.exec();

    getApp()->accounts->twitch.currentUsername = username;
}

}  // namespace

BasicLoginWidget::BasicLoginWidget()
{
    this->setLayout(&this->ui.layout);

    this->ui.loginButton.setText("Log in (Opens in browser)");
    this->ui.pasteCodeButton.setText("Paste code");

    this->ui.horizontalLayout.addWidget(&this->ui.loginButton);
    this->ui.horizontalLayout.addWidget(&this->ui.pasteCodeButton);

    this->ui.layout.addLayout(&this->ui.horizontalLayout);

    connect(&this->ui.loginButton, &QPushButton::clicked, []() {
        printf("open login in browser\n");
        QDesktopServices::openUrl(QUrl("https://chatterino.com/client_login"));
    });

    connect(&this->ui.pasteCodeButton, &QPushButton::clicked, [this]() {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QString clipboardString = clipboard->text();
        QStringList parameters = clipboardString.split(';');

        std::string oauthToken, clientID, username, userID;

        for (const auto &param : parameters) {
            QStringList kvParameters = param.split('=');
            if (kvParameters.size() != 2) {
                continue;
            }
            QString key = kvParameters[0];
            QString value = kvParameters[1];

            if (key == "oauth_token") {
                oauthToken = value.toStdString();
            } else if (key == "client_id") {
                clientID = value.toStdString();
            } else if (key == "username") {
                username = value.toStdString();
            } else if (key == "user_id") {
                userID = value.toStdString();
            } else {
                qDebug() << "Unknown key in code: " << key;
            }
        }

        LogInWithCredentials(userID, username, clientID, oauthToken);

        clipboard->clear();
        this->window()->close();
    });
}

AdvancedLoginWidget::AdvancedLoginWidget()
{
    this->setLayout(&this->ui.layout);

    this->ui.instructionsLabel.setText("1. Fill in your username\n2. Fill in your user ID or press "
                                       "the 'Get user ID from username' button\n3. Fill in your "
                                       "Client ID\n4. Fill in your OAuth Token\n5. Press Add User");
    this->ui.instructionsLabel.setWordWrap(true);

    this->ui.layout.addWidget(&this->ui.instructionsLabel);
    this->ui.layout.addLayout(&this->ui.formLayout);
    this->ui.layout.addLayout(&this->ui.buttonUpperRow.layout);
    this->ui.layout.addLayout(&this->ui.buttonLowerRow.layout);

    this->refreshButtons();

    /// Form
    this->ui.formLayout.addRow("Username", &this->ui.usernameInput);
    this->ui.formLayout.addRow("User ID", &this->ui.userIDInput);
    this->ui.formLayout.addRow("Client ID", &this->ui.clientIDInput);
    this->ui.formLayout.addRow("Oauth token", &this->ui.oauthTokenInput);

    this->ui.oauthTokenInput.setEchoMode(QLineEdit::Password);

    connect(&this->ui.userIDInput, &QLineEdit::textChanged, [=]() { this->refreshButtons(); });
    connect(&this->ui.usernameInput, &QLineEdit::textChanged, [=]() { this->refreshButtons(); });
    connect(&this->ui.clientIDInput, &QLineEdit::textChanged, [=]() { this->refreshButtons(); });
    connect(&this->ui.oauthTokenInput, &QLineEdit::textChanged, [=]() { this->refreshButtons(); });

    /// Upper button row

    this->ui.buttonUpperRow.addUserButton.setText("Add user");
    this->ui.buttonUpperRow.clearFieldsButton.setText("Clear fields");

    this->ui.buttonUpperRow.layout.addWidget(&this->ui.buttonUpperRow.addUserButton);
    this->ui.buttonUpperRow.layout.addWidget(&this->ui.buttonUpperRow.clearFieldsButton);

    connect(&this->ui.buttonUpperRow.clearFieldsButton, &QPushButton::clicked, [=]() {
        this->ui.userIDInput.clear();
        this->ui.usernameInput.clear();
        this->ui.clientIDInput.clear();
        this->ui.oauthTokenInput.clear();
    });

    connect(&this->ui.buttonUpperRow.addUserButton, &QPushButton::clicked, [=]() {
        std::string userID = this->ui.userIDInput.text().toStdString();
        std::string username = this->ui.usernameInput.text().toStdString();
        std::string clientID = this->ui.clientIDInput.text().toStdString();
        std::string oauthToken = this->ui.oauthTokenInput.text().toStdString();

        LogInWithCredentials(userID, username, clientID, oauthToken);
    });

    /// Lower button row
    this->ui.buttonLowerRow.fillInUserIDButton.setText("Get user ID from username");

    this->ui.buttonLowerRow.layout.addWidget(&this->ui.buttonLowerRow.fillInUserIDButton);

    connect(&this->ui.buttonLowerRow.fillInUserIDButton, &QPushButton::clicked, [=]() {
        util::twitch::getUserID(this->ui.usernameInput.text(), this, [=](const QString &userID) {
            this->ui.userIDInput.setText(userID);  //
        });
    });
}

void AdvancedLoginWidget::refreshButtons()
{
    this->ui.buttonLowerRow.fillInUserIDButton.setEnabled(!this->ui.usernameInput.text().isEmpty());

    if (this->ui.userIDInput.text().isEmpty() || this->ui.usernameInput.text().isEmpty() ||
        this->ui.clientIDInput.text().isEmpty() || this->ui.oauthTokenInput.text().isEmpty()) {
        this->ui.buttonUpperRow.addUserButton.setEnabled(false);
    } else {
        this->ui.buttonUpperRow.addUserButton.setEnabled(true);
    }
}

LoginWidget::LoginWidget()
{
#ifdef USEWINSDK
    ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif

    this->setLayout(&this->ui.mainLayout);

    this->ui.mainLayout.addWidget(&this->ui.tabWidget);

    this->ui.tabWidget.addTab(&this->ui.basic, "Basic");

    this->ui.tabWidget.addTab(&this->ui.advanced, "Advanced");

    this->ui.buttonBox.setStandardButtons(QDialogButtonBox::Close);

    QObject::connect(&this->ui.buttonBox, &QDialogButtonBox::rejected, [this]() {
        this->close();  //
    });

    this->ui.mainLayout.addWidget(&this->ui.buttonBox);
}

}  // namespace widgets
}  // namespace chatterino
