#include "widgets/dialogs/LoginDialog.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "util/Clipboard.hpp"
#include "util/Helpers.hpp"
#include "widgets/helper/LoginServer.hpp"

#ifdef USEWINSDK
#    include <Windows.h>
#endif

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <pajlada/settings/setting.hpp>

namespace chatterino {

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , loginServer_(new LoginServer(this))
    // By default, QUrl constructor urlencodes our string so we don't have to do this ourselves
    , loginLink(QString("https://id.twitch.tv/oauth2/authorize"
                        "?client_id=%1"
                        "&redirect_uri=%2"
                        "&response_type=%3"
                        "&scope=%4"
                        "&force_verify=%5")
                    .arg(getDefaultClientID(), getRedirectURI(), "token",
                         loginScopes.join(" "), QVariant(true).toString()))
{
#ifdef USEWINSDK
    ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif

    this->setWindowTitle("Chatterino - add new account");
    this->setLayout(&this->ui_.layout);

    // Label with explanation of what does user have to do here
    this->ui_.helpLabel.setText(
        QString("Click on the \"Log in\" button to open <a href=\"%1\">Twitch "
                "login page</a> in browser.<br>If it doesn't open "
                "automatically, right click the link and open it yourself."
                "<br><br>You can also paste your token from clipboard.")
            .arg(this->loginLink.toString()));
    this->ui_.helpLabel.setOpenExternalLinks(true);

    // Label that ensures this is babyproof
    this->ui_.warningLabel.setText("DO NOT SHOW THIS ON STREAM!!!");
    this->ui_.warningLabel.setStyleSheet(
        "QLabel { color: red; font-weight: 900;"
        "font-size: 3em; text-align: center; }");

    // Login buttons
    this->ui_.loginButton.setText(LOGIN_BUTTON_START);
    this->ui_.pasteTokenButton.setText(TOKEN_BUTTON_START);

    // Separate box with Close button
    this->ui_.buttonBox.setStandardButtons(QDialogButtonBox::Close);

    // Add everything to layout
    this->ui_.layout.addWidget(&this->ui_.helpLabel);
    this->ui_.layout.addWidget(&this->ui_.warningLabel);

    this->ui_.buttons.addWidget(&this->ui_.loginButton);
    this->ui_.buttons.addWidget(&this->ui_.pasteTokenButton);
    this->ui_.layout.addLayout(&this->ui_.buttons);

    this->ui_.layout.addWidget(&this->ui_.buttonBox);

    // Connect to button events
    QObject::connect(&this->ui_.buttonBox, &QDialogButtonBox::rejected, [this] {
        this->close();
    });

    QObject::connect(&this->ui_.loginButton, &QPushButton::clicked, [this] {
        // Start listening for credentials
        if (!this->loginServer_->listen())
        {
            qCWarning(chatterinoWidget) << "Failed to start HTTP server";
        }
        else
        {
            qInfo(chatterinoWidget) << "HTTP server Listening on " +
                                           this->loginServer_->getAddress();
        }

        // Open login page
        QDesktopServices::openUrl(this->loginLink);
        this->deactivateLoginButton();
    });

    QObject::connect(&this->ui_.pasteTokenButton, &QPushButton::clicked, [this] {
        this->ui_.pasteTokenButton.setText(VALIDATING_TOKEN);
        this->ui_.pasteTokenButton.setDisabled(true);
        this->logInWithToken(
            getClipboardText(),
            [] {
                // success
                // Removing clipboard content to prevent accidental paste of credentials into somewhere
                crossPlatformCopy(QString());
            },
            [this] {
                // failure
                this->ui_.pasteTokenButton.setText("Invalid token pasted!");
            },
            [this] {
                // finally
                // Add some sort of "rate-limit" to not spam logInWithToken
                QTimer::singleShot(3000, [this] {
                    this->ui_.pasteTokenButton.setText(TOKEN_BUTTON_START);
                    this->ui_.pasteTokenButton.setEnabled(true);
                });
            });
    });
}

void LoginDialog::deactivateLoginButton()
{
    this->ui_.loginButton.setText("Continue in browser...");
    this->ui_.loginButton.setDisabled(true);
}

void LoginDialog::activateLoginButton()
{
    this->ui_.loginButton.setText(LOGIN_BUTTON_START);
    this->ui_.loginButton.setEnabled(true);
}

void LoginDialog::logInWithToken(QString token,
                                 std::function<void()> successCallback,
                                 std::function<void()> failureCallback,
                                 std::function<void()> finallyCallback)
{
    NetworkRequest("https://id.twitch.tv/oauth2/validate")
        .timeout(5 * 1000)
        .header("Accept", "application/json")
        .header("Authorization", "OAuth " + token)
        .onSuccess([successCallback, failureCallback, token,
                    this](NetworkResult result) -> Outcome {
            auto root = result.parseJson();
            TokenValidationResponse validation(root);

            if (validation.clientId.isEmpty() || validation.login.isEmpty() ||
                validation.userId.isEmpty())
            {
                failureCallback();
                return Failure;
            }

            // Update account settings and call it a success
            std::string basePath =
                "/accounts/uid" + validation.userId.toStdString();
            pajlada::Settings::Setting<QString>::set(basePath + "/username",
                                                     validation.login);
            pajlada::Settings::Setting<QString>::set(basePath + "/userID",
                                                     validation.userId);
            pajlada::Settings::Setting<QString>::set(basePath + "/clientID",
                                                     validation.clientId);
            pajlada::Settings::Setting<QString>::set(basePath + "/oauthToken",
                                                     token);

            getApp()->accounts->twitch.reloadUsers();
            getApp()->accounts->twitch.currentUsername = validation.login;
            // Closing the window will emit hideEvent which will close the server
            // however, any already extablished connections will not terminate immidiatelly,
            // so it's fine to close the window already
            this->window()->close();

            successCallback();
            return Success;
        })
        .onError([failureCallback](auto result) {
            failureCallback();
        })
        .finally(finallyCallback)
        .execute();
}

void LoginDialog::hideEvent(QHideEvent *event)
{
    // Make the port free
    this->loginServer_->close();
    // Restore login button
    this->activateLoginButton();
}

}  // namespace chatterino
