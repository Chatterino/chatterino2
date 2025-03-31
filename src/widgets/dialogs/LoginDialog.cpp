#include "widgets/dialogs/LoginDialog.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "singletons/Settings.hpp"
#include "util/Clipboard.hpp"
#include "util/Helpers.hpp"

#ifdef USEWINSDK
#    include <Windows.h>
#endif

#include <pajlada/settings/setting.hpp>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

namespace chatterino {

namespace {

    bool logInWithCredentials(QWidget *parent, const QString &userID,
                              const QString &username, const QString &clientID,
                              const QString &oauthToken)
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
            QMessageBox messageBox(parent);
            messageBox.setWindowTitle("Invalid account credentials");
            messageBox.setIcon(QMessageBox::Critical);
            messageBox.setText(errors.join("<br>"));
            messageBox.exec();
            return false;
        }

        std::string basePath = "/accounts/uid" + userID.toStdString();
        pajlada::Settings::Setting<QString>::set(basePath + "/username",
                                                 username);
        pajlada::Settings::Setting<QString>::set(basePath + "/userID", userID);
        pajlada::Settings::Setting<QString>::set(basePath + "/clientID",
                                                 clientID);
        pajlada::Settings::Setting<QString>::set(basePath + "/oauthToken",
                                                 oauthToken);

        getApp()->getAccounts()->twitch.reloadUsers();
        getApp()->getAccounts()->twitch.currentUsername = username;
        getSettings()->requestSave();
        return true;
    }

}  // namespace

BasicLoginWidget::BasicLoginWidget()
{
    const QString logInLink = "https://chatterino.com/client_login";
    this->setLayout(&this->ui_.layout);

    this->ui_.loginButton.setText("Log in (Opens in browser)");
    this->ui_.pasteCodeButton.setText("Paste login info");
    this->ui_.unableToOpenBrowserHelper.setWindowTitle(
        "Chatterino - unable to open in browser");
    this->ui_.unableToOpenBrowserHelper.setWordWrap(true);
    this->ui_.unableToOpenBrowserHelper.hide();
    this->ui_.unableToOpenBrowserHelper.setText(
        QString("An error occurred while attempting to open <a href=\"%1\">the "
                "log in link (%1)</a> - open it manually in your browser and "
                "proceed from there.")
            .arg(logInLink));
    this->ui_.unableToOpenBrowserHelper.setOpenExternalLinks(true);

    this->ui_.horizontalLayout.addWidget(&this->ui_.loginButton);
    this->ui_.horizontalLayout.addWidget(&this->ui_.pasteCodeButton);

    this->ui_.layout.addLayout(&this->ui_.horizontalLayout);
    this->ui_.layout.addWidget(&this->ui_.unableToOpenBrowserHelper);

    connect(&this->ui_.loginButton, &QPushButton::clicked, [this, logInLink]() {
        qCDebug(chatterinoWidget) << "open login in browser";
        if (!QDesktopServices::openUrl(QUrl(logInLink)))
        {
            qCWarning(chatterinoWidget) << "open login in browser failed";
            this->ui_.unableToOpenBrowserHelper.show();
        }
    });

    connect(&this->ui_.pasteCodeButton, &QPushButton::clicked, [this]() {
        QStringList parameters = getClipboardText().split(";");
        QString oauthToken, clientID, username, userID;

        // Removing clipboard content to prevent accidental paste of credentials into somewhere
        crossPlatformCopy("");

        for (const auto &param : parameters)
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
                qCWarning(chatterinoWidget) << "Unknown key in code: " << key;
            }
        }

        if (logInWithCredentials(this, userID, username, clientID, oauthToken))
        {
            this->window()->close();
        }
    });
}

AdvancedLoginWidget::AdvancedLoginWidget()
{
    this->setLayout(&this->ui_.layout);

    this->ui_.instructionsLabel.setText("1. Fill in your username"
                                        "\n2. Fill in your user ID"
                                        "\n3. Fill in your client ID"
                                        "\n4. Fill in your OAuth token"
                                        "\n5. Press Add user");
    this->ui_.instructionsLabel.setWordWrap(true);

    this->ui_.layout.addWidget(&this->ui_.instructionsLabel);
    this->ui_.layout.addLayout(&this->ui_.formLayout);
    this->ui_.layout.addLayout(&this->ui_.buttonUpperRow.layout);

    this->refreshButtons();

    /// Form
    this->ui_.formLayout.addRow("Username", &this->ui_.usernameInput);
    this->ui_.formLayout.addRow("User ID", &this->ui_.userIDInput);
    this->ui_.formLayout.addRow("Client ID", &this->ui_.clientIDInput);
    this->ui_.formLayout.addRow("OAuth token", &this->ui_.oauthTokenInput);

    this->ui_.oauthTokenInput.setEchoMode(QLineEdit::Password);

    connect(&this->ui_.userIDInput, &QLineEdit::textChanged, [this]() {
        this->refreshButtons();
    });
    connect(&this->ui_.usernameInput, &QLineEdit::textChanged, [this]() {
        this->refreshButtons();
    });
    connect(&this->ui_.clientIDInput, &QLineEdit::textChanged, [this]() {
        this->refreshButtons();
    });
    connect(&this->ui_.oauthTokenInput, &QLineEdit::textChanged, [this]() {
        this->refreshButtons();
    });

    /// Upper button row

    this->ui_.buttonUpperRow.addUserButton.setText("Add user");
    this->ui_.buttonUpperRow.clearFieldsButton.setText("Clear fields");

    this->ui_.buttonUpperRow.layout.addWidget(
        &this->ui_.buttonUpperRow.addUserButton);
    this->ui_.buttonUpperRow.layout.addWidget(
        &this->ui_.buttonUpperRow.clearFieldsButton);

    connect(&this->ui_.buttonUpperRow.clearFieldsButton, &QPushButton::clicked,
            [this]() {
                this->ui_.userIDInput.clear();
                this->ui_.usernameInput.clear();
                this->ui_.clientIDInput.clear();
                this->ui_.oauthTokenInput.clear();
            });

    connect(&this->ui_.buttonUpperRow.addUserButton, &QPushButton::clicked,
            [this]() {
                QString userID = this->ui_.userIDInput.text();
                QString username = this->ui_.usernameInput.text();
                QString clientID = this->ui_.clientIDInput.text();
                QString oauthToken = this->ui_.oauthTokenInput.text();

                logInWithCredentials(this, userID, username, clientID,
                                     oauthToken);
            });
}

void AdvancedLoginWidget::refreshButtons()
{
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

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    this->setMinimumWidth(300);
    this->setWindowFlags(
        (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    this->setWindowTitle("Add new account");

    this->setLayout(&this->ui_.mainLayout);
    this->ui_.mainLayout.addWidget(&this->ui_.tabWidget);

    this->ui_.tabWidget.addTab(&this->ui_.basic, "Basic");
    this->ui_.tabWidget.addTab(&this->ui_.advanced, "Advanced");

    this->ui_.buttonBox.setStandardButtons(QDialogButtonBox::Close);

    QObject::connect(&this->ui_.buttonBox, &QDialogButtonBox::rejected,
                     [this]() {
                         this->close();
                     });

    this->ui_.mainLayout.addWidget(&this->ui_.buttonBox);
}

}  // namespace chatterino
