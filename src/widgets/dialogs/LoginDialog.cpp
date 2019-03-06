#include "widgets/dialogs/LoginDialog.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/PartialTwitchUser.hpp"
#include "util/Helpers.hpp"

#ifdef USEWINSDK
#    include <Windows.h>
#endif

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <pajlada/settings/setting.hpp>

namespace chatterino
{
    namespace
    {
        void LogInWithCredentials(const QString& userID,
            const QString& username, const QString& clientID,
            const QString& oauthToken)
        {
            QStringList errors;

            if (userID.isEmpty())
            {
                errors.append("Missing user ID");
            }
            if (username.isEmpty())
            {
                errors.append("Missing username");
            }
            if (clientID.isEmpty())
            {
                errors.append("Missing Client ID");
            }
            if (oauthToken.isEmpty())
            {
                errors.append("Missing OAuth Token");
            }

            if (errors.length() > 0)
            {
                QMessageBox messageBox;
// Set error window on top
#ifdef USEWINSDK
                ::SetWindowPos(HWND(messageBox.winId()), HWND_TOPMOST, 0, 0, 0,
                    0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

#endif
                messageBox.setIcon(QMessageBox::Critical);
                messageBox.setText(errors.join("<br />"));
                messageBox.setStandardButtons(QMessageBox::Ok);
                messageBox.exec();
                return;
            }

            //    QMessageBox messageBox;
            //    messageBox.setIcon(QMessageBox::Information);
            //    messageBox.setText("Successfully logged in with user <b>" +
            //    qS(username) + "</b>!");
            auto basePath = fS("/accounts/uid{}", userID);
            pajlada::Settings::Setting<QString>::set(
                basePath + "/username", username);
            pajlada::Settings::Setting<QString>::set(
                basePath + "/userID", userID);
            pajlada::Settings::Setting<QString>::set(
                basePath + "/clientID", clientID);
            pajlada::Settings::Setting<QString>::set(
                basePath + "/oauthToken", oauthToken);

            getApp()->accounts->twitch.reloadUsers();

            //    messageBox.exec();

            getApp()->accounts->twitch.currentUsername = username;
        }

    }  // namespace

    BasicLoginWidget::BasicLoginWidget()
    {
        this->setLayout(&this->ui_.layout);

        this->ui_.loginButton.setText("Log in (Opens in browser)");
        this->ui_.pasteCodeButton.setText("Paste code");

        this->ui_.horizontalLayout.addWidget(&this->ui_.loginButton);
        this->ui_.horizontalLayout.addWidget(&this->ui_.pasteCodeButton);

        this->ui_.layout.addLayout(&this->ui_.horizontalLayout);

        connect(&this->ui_.loginButton, &QPushButton::clicked, []() {
            printf("open login in browser\n");
            QDesktopServices::openUrl(
                QUrl("https://chatterino.com/client_login"));
        });

        connect(&this->ui_.pasteCodeButton, &QPushButton::clicked, [this]() {
            QClipboard* clipboard = QGuiApplication::clipboard();
            QString clipboardString = clipboard->text();
            QStringList parameters = clipboardString.split(';');

            QString oauthToken, clientID, username, userID;

            for (const auto& param : parameters)
            {
                QStringList kvParameters = param.split('=');
                if (kvParameters.size() != 2)
                {
                    continue;
                }
                QString key = kvParameters[0];
                QString value = kvParameters[1];

                if (key == "oauth_token")
                {
                    oauthToken = value;
                }
                else if (key == "client_id")
                {
                    clientID = value;
                }
                else if (key == "username")
                {
                    username = value;
                }
                else if (key == "user_id")
                {
                    userID = value;
                }
                else
                {
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
        this->setLayout(&this->ui_.layout);

        this->ui_.instructionsLabel.setText(
            "1. Fill in your username\n2. Fill in your user ID or press "
            "the 'Get user ID from username' button\n3. Fill in your "
            "Client ID\n4. Fill in your OAuth Token\n5. Press Add User");
        this->ui_.instructionsLabel.setWordWrap(true);

        this->ui_.layout.addWidget(&this->ui_.instructionsLabel);
        this->ui_.layout.addLayout(&this->ui_.formLayout);
        this->ui_.layout.addLayout(&this->ui_.buttonUpperRow.layout);
        this->ui_.layout.addLayout(&this->ui_.buttonLowerRow.layout);

        this->refreshButtons();

        /// Form
        this->ui_.formLayout.addRow("Username", &this->ui_.usernameInput);
        this->ui_.formLayout.addRow("User ID", &this->ui_.userIDInput);
        this->ui_.formLayout.addRow("Client ID", &this->ui_.clientIDInput);
        this->ui_.formLayout.addRow("Oauth token", &this->ui_.oauthTokenInput);

        this->ui_.oauthTokenInput.setEchoMode(QLineEdit::Password);

        connect(&this->ui_.userIDInput, &QLineEdit::textChanged,
            [=]() { this->refreshButtons(); });
        connect(&this->ui_.usernameInput, &QLineEdit::textChanged,
            [=]() { this->refreshButtons(); });
        connect(&this->ui_.clientIDInput, &QLineEdit::textChanged,
            [=]() { this->refreshButtons(); });
        connect(&this->ui_.oauthTokenInput, &QLineEdit::textChanged,
            [=]() { this->refreshButtons(); });

        /// Upper button row

        this->ui_.buttonUpperRow.addUserButton.setText("Add user");
        this->ui_.buttonUpperRow.clearFieldsButton.setText("Clear fields");

        this->ui_.buttonUpperRow.layout.addWidget(
            &this->ui_.buttonUpperRow.addUserButton);
        this->ui_.buttonUpperRow.layout.addWidget(
            &this->ui_.buttonUpperRow.clearFieldsButton);

        connect(&this->ui_.buttonUpperRow.clearFieldsButton,
            &QPushButton::clicked, [=]() {
                this->ui_.userIDInput.clear();
                this->ui_.usernameInput.clear();
                this->ui_.clientIDInput.clear();
                this->ui_.oauthTokenInput.clear();
            });

        connect(&this->ui_.buttonUpperRow.addUserButton, &QPushButton::clicked,
            [=]() {
                QString userID = this->ui_.userIDInput.text();
                QString username = this->ui_.usernameInput.text();
                QString clientID = this->ui_.clientIDInput.text();
                QString oauthToken = this->ui_.oauthTokenInput.text();

                LogInWithCredentials(userID, username, clientID, oauthToken);
            });

        /// Lower button row
        this->ui_.buttonLowerRow.fillInUserIDButton.setText(
            "Get user ID from username");

        this->ui_.buttonLowerRow.layout.addWidget(
            &this->ui_.buttonLowerRow.fillInUserIDButton);

        connect(&this->ui_.buttonLowerRow.fillInUserIDButton,
            &QPushButton::clicked, [=]() {
                const auto onIdFetched = [=](const QString& userID) {
                    this->ui_.userIDInput.setText(userID);  //
                };
                PartialTwitchUser::byName(this->ui_.usernameInput.text())
                    .getId(onIdFetched, this);
            });
    }

    void AdvancedLoginWidget::refreshButtons()
    {
        this->ui_.buttonLowerRow.fillInUserIDButton.setEnabled(
            !this->ui_.usernameInput.text().isEmpty());

        if (this->ui_.userIDInput.text().isEmpty() ||
            this->ui_.usernameInput.text().isEmpty() ||
            this->ui_.clientIDInput.text().isEmpty() ||
            this->ui_.oauthTokenInput.text().isEmpty())
        {
            this->ui_.buttonUpperRow.addUserButton.setEnabled(false);
        }
        else
        {
            this->ui_.buttonUpperRow.addUserButton.setEnabled(true);
        }
    }

    LoginWidget::LoginWidget()
    {
#ifdef USEWINSDK
        ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif

        this->setLayout(&this->ui_.mainLayout);

        this->ui_.mainLayout.addWidget(&this->ui_.tabWidget);

        this->ui_.tabWidget.addTab(&this->ui_.basic, "Basic");

        this->ui_.tabWidget.addTab(&this->ui_.advanced, "Advanced");

        this->ui_.buttonBox.setStandardButtons(QDialogButtonBox::Close);

        QObject::connect(
            &this->ui_.buttonBox, &QDialogButtonBox::rejected, [this]() {
                this->close();  //
            });

        this->ui_.mainLayout.addWidget(&this->ui_.buttonBox);
    }

}  // namespace chatterino
