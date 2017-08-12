#include "widgets/logindialog.hpp"

#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <pajlada/settings/setting.hpp>

namespace chatterino {
namespace widgets {

LoginWidget::LoginWidget()
{
    this->setLayout(&this->ui.mainLayout);

    this->ui.loginButton.setText("Log in (Opens in browser)");
    this->ui.pasteCodeButton.setText("Paste code");

    this->ui.horizontalLayout.addWidget(&this->ui.loginButton);
    this->ui.horizontalLayout.addWidget(&this->ui.pasteCodeButton);

    this->ui.verticalLayout.addLayout(&this->ui.horizontalLayout);

    this->ui.mainLayout.addLayout(&this->ui.verticalLayout);

    this->ui.buttonBox.setStandardButtons(QDialogButtonBox::Close);

    this->ui.mainLayout.addWidget(&this->ui.buttonBox);

    connect(&this->ui.buttonBox, &QDialogButtonBox::rejected, [this]() {
        this->close();  //
    });

    connect(&this->ui.loginButton, &QPushButton::clicked, []() {
        printf("open login in browser\n");
        QDesktopServices::openUrl(QUrl("https://pajlada.se/chatterino/#chatterino"));

    });

    connect(&this->ui.pasteCodeButton, &QPushButton::clicked, []() {
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

        if (oauthToken.empty() || clientID.empty() || username.empty() || userID.empty()) {
            qDebug() << "Missing variables!!!!!!!!!";
        } else {
            qDebug() << "Success! mr";
            pajlada::Settings::Setting<std::string>::set("/accounts/uid" + userID + "/username",
                                                         username);
            pajlada::Settings::Setting<std::string>::set("/accounts/uid" + userID + "/userID",
                                                         userID);
            pajlada::Settings::Setting<std::string>::set("/accounts/uid" + userID + "/clientID",
                                                         clientID);
            pajlada::Settings::Setting<std::string>::set("/accounts/uid" + userID + "/oauthToken",
                                                         oauthToken);
        }
    });
}

}  // namespace widgets
}  // namespace chatterino
