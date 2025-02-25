#include "widgets/dialogs/LoginDialog.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Literals.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "util/Clipboard.hpp"
#include "util/Helpers.hpp"

#ifdef USEWINSDK
#    include <Windows.h>
#endif

#include <pajlada/settings/setting.hpp>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QMessageBox>
#include <QPointer>
#include <QUrl>
#include <QUrlQuery>

namespace {

using namespace chatterino;
using namespace literals;

bool logInWithImplicitGrantCredentials(QWidget *parent, const QString &userID,
                                       const QString &username,
                                       const QString &clientID,
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

    if (!errors.empty())
    {
        QMessageBox messageBox(parent);
        messageBox.setWindowTitle("Invalid account credentials");
        messageBox.setIcon(QMessageBox::Critical);
        messageBox.setText(errors.join("<br>"));
        messageBox.exec();
        return false;
    }

    TwitchAccountData{
        .username = username,
        .userID = userID,
        .clientID = clientID,
        .oauthToken = oauthToken,
        .ty = TwitchAccount::Type::ImplicitGrant,
        .refreshToken = {},
        .expiresAt = {},
    }
        .save();

    getApp()->getAccounts()->twitch.reloadUsers();
    getApp()->getAccounts()->twitch.currentUsername = username;
    getSettings()->requestSave();
    return true;
}

class DeviceLoginJob;
class DeviceLoginWidget : public QWidget
{
public:
    DeviceLoginWidget();

    void reset(const QString &prevError = {});
    void displayError(const QString &error);

private:
    void tryInitSession(const QJsonObject &response);

    void updateCurrentWidget(QWidget *next);

    QHBoxLayout layout;
    QLabel *detailLabel = nullptr;

    QString verificationUri_;
    QString userCode_;
    QString scopes_;

    DeviceLoginJob *job_ = nullptr;
};

class DeviceLoginJob : public QObject
{
public:
    struct Session {
        QString deviceCode;
        QString scopes;
        std::chrono::milliseconds expiry;
        std::chrono::milliseconds interval;
    };

    DeviceLoginJob(DeviceLoginWidget *ui, Session session);

private:
    void ping();

    QPointer<DeviceLoginWidget> ui;
    QTimer expiryTimer_;
    QTimer pingTimer_;
    Session session_;
};

DeviceLoginWidget::DeviceLoginWidget()
{
    this->setLayout(&this->layout);
    this->reset();
}

void DeviceLoginWidget::updateCurrentWidget(QWidget *next)
{
    // clear the layout
    QLayoutItem *prev = nullptr;
    while ((prev = this->layout.takeAt(0)))
    {
        delete prev->widget();
        delete prev;
    }

    // insert the item
    this->layout.addWidget(next, 1, Qt::AlignCenter);
}

void DeviceLoginWidget::reset(const QString &prevError)
{
    if (this->job_)
    {
        this->job_->deleteLater();
        assert(this->job_ == nullptr);
    }

    auto *wrap = new QWidget;
    auto *layout = new QVBoxLayout(wrap);

    auto *titleLabel = new QLabel(u"Click on 'Start' to connect an account!"_s);
    this->detailLabel = new QLabel(prevError);
    this->detailLabel->setWordWrap(true);
    layout->addWidget(titleLabel, 1, Qt::AlignCenter);
    layout->addWidget(this->detailLabel, 0, Qt::AlignCenter);

    this->scopes_ = QString{};
    for (auto scope : DEVICE_AUTH_SCOPES)
    {
        if (!this->scopes_.isEmpty())
        {
            this->scopes_.append(' ');
        }
        this->scopes_.append(scope);
    }

    auto *startButton = new QPushButton(u"Start"_s);
    connect(startButton, &QPushButton::clicked, this, [this] {
        QUrlQuery query{
            {u"client_id"_s, DEVICE_AUTH_CLIENT_ID},
            {u"scopes"_s, this->scopes_},
        };
        NetworkRequest(u"https://id.twitch.tv/oauth2/device"_s,
                       NetworkRequestType::Post)
            .payload(query.toString(QUrl::FullyEncoded).toUtf8())
            .timeout(10000)
            .caller(this)
            .onSuccess([this](const auto &res) {
                this->tryInitSession(res.parseJson());
                return Success;
            })
            .onError([this](const auto &res) {
                const auto json = res.parseJson();
                this->displayError(
                    json["message"_L1].toString(u"error: (no message)"_s));
            })
            .execute();
    });
    layout->addWidget(startButton);

    this->updateCurrentWidget(wrap);
}

void DeviceLoginWidget::tryInitSession(const QJsonObject &response)
{
    auto getString = [&](auto key, QString &dest) {
        const auto val = response[key];
        if (!val.isString())
        {
            return false;
        }
        dest = val.toString();
        return true;
    };
    QString deviceCode;
    if (!getString("device_code"_L1, deviceCode))
    {
        this->displayError(u"Failed to initialize: missing 'device_code'"_s);
        return;
    }
    if (!getString("user_code"_L1, this->userCode_))
    {
        this->displayError(u"Failed to initialize: missing 'user_code'"_s);
        return;
    }
    if (!getString("verification_uri"_L1, this->verificationUri_))
    {
        this->displayError(
            u"Failed to initialize: missing 'verification_uri'"_s);
        return;
    }

    if (this->job_)
    {
        assert(false && "There shouldn't be any job at this point");
        this->job_->deleteLater();
    }
    this->job_ = new DeviceLoginJob(
        this,
        {
            .deviceCode = deviceCode,
            .scopes = this->scopes_,
            .expiry =
                std::chrono::seconds(response["expires_in"_L1].toInt(1800)),
            .interval = std::chrono::seconds(response["interval"_L1].toInt(5)),
        });
    QObject::connect(this->job_, &QObject::destroyed, this, [this] {
        this->job_ = nullptr;
    });

    auto *wrap = new QWidget;
    auto *layout = new QVBoxLayout(wrap);

    // A simplified link split by the code, such that
    // prefixUrl is the part before the code and postfixUrl
    // is the part after the code.
    auto [prefixUrl, postfixUrl] = [&] {
        QStringView view(this->verificationUri_);
        // TODO(Qt 6): use .sliced()
        if (view.startsWith(u"https://"))
        {
            view = view.mid(8);
        }
        if (view.startsWith(u"www."))
        {
            view = view.mid(4);
        }

        auto idx = view.indexOf(this->userCode_);
        if (idx < 0)
        {
            return std::make_tuple(view, QStringView());
        }

        return std::make_tuple(view.mid(0, idx),
                               view.mid(idx + this->userCode_.length()));
    }();

    // <a href={verificationUri}>
    //   <span>{prefixUrl}</span>
    //   <span style="font-size: large">{userCode}</span>
    //   <span>{postfixUrl}</span>
    // </a>
    auto *userCode = new QLabel(
        u"<a href=\"%1\"><span>%2</span><span style=\"font-size: large\">%3</span><span>%4</span></a>"_s
            .arg(this->verificationUri_, prefixUrl, this->userCode_,
                 postfixUrl));
    userCode->setOpenExternalLinks(true);
    userCode->setTextInteractionFlags(Qt::TextBrowserInteraction);
    if (getApp()->getStreamerMode()->isEnabled())
    {
        userCode->setText(
            u"You're in streamer mode.\nUse the buttons below.\nDon't show the code on stream!"_s);
    }
    layout->addWidget(userCode, 1, Qt::AlignCenter);

    this->detailLabel = new QLabel;
    layout->addWidget(this->detailLabel, 0, Qt::AlignCenter);

    {
        auto *hbox = new QHBoxLayout;

        auto addButton = [&](const auto &text, auto handler) {
            auto *button = new QPushButton(text);
            connect(button, &QPushButton::clicked, handler);
            hbox->addWidget(button, 1);
        };
        addButton(u"Copy code"_s, [this] {
            crossPlatformCopy(this->userCode_);
        });
        addButton(u"Copy URL"_s, [this] {
            crossPlatformCopy(this->verificationUri_);
        });
        addButton(u"Open URL"_s, [this] {
            if (!QDesktopServices::openUrl(QUrl(this->verificationUri_)))
            {
                qCWarning(chatterinoWidget) << "open login in browser failed";
                this->displayError(u"Failed to open browser"_s);
            }
        });
        layout->addLayout(hbox, 1);
    }

    this->updateCurrentWidget(wrap);
}

void DeviceLoginWidget::displayError(const QString &error)
{
    if (this->detailLabel)
    {
        this->detailLabel->setText(error);
    }
    else
    {
        qCWarning(chatterinoWidget)
            << "Tried to display error but no detail label was found - error:"
            << error;
    }
}

DeviceLoginJob::DeviceLoginJob(DeviceLoginWidget *ui, Session session)
    : ui(ui)
    , session_(std::move(session))
{
    QObject::connect(&this->expiryTimer_, &QTimer::timeout, this,
                     &QObject::deleteLater);
    QObject::connect(&this->pingTimer_, &QTimer::timeout, this,
                     &DeviceLoginJob::ping);

    this->expiryTimer_.start(this->session_.expiry);
    this->pingTimer_.start(this->session_.interval);
};

void DeviceLoginJob::ping()
{
    QUrlQuery query{
        {u"client_id"_s, DEVICE_AUTH_CLIENT_ID},
        {u"scope"_s, this->session_.scopes},
        {u"device_code"_s, this->session_.deviceCode},
        {u"grant_type"_s, u"urn:ietf:params:oauth:grant-type:device_code"_s},
    };

    NetworkRequest(u"https://id.twitch.tv/oauth2/token"_s,
                   NetworkRequestType::Post)
        .caller(this)
        .timeout((this->pingTimer_.interval() * 9) / 10)
        .payload(query.toString(QUrl::FullyEncoded).toUtf8())
        .onSuccess([this](const auto &res) {
            const auto json = res.parseJson();
            auto accessToken = json["access_token"_L1].toString();
            auto refreshToken = json["refresh_token"_L1].toString();
            auto expiresIn = json["expires_in"_L1].toInt(-1);
            if (accessToken.isEmpty() || refreshToken.isEmpty() ||
                expiresIn <= 0)
            {
                if (this->ui)
                {
                    this->ui->displayError("Received malformed response");
                }
                return;
            }
            auto expiresAt =
                QDateTime::currentDateTimeUtc().addSecs(expiresIn - 120);
            QPointer self(this);
            auto helix = std::make_shared<Helix>();
            helix->update(DEVICE_AUTH_CLIENT_ID, accessToken);
            helix->fetchUsers(
                {}, {},
                [self, helix, refreshToken, accessToken,
                 expiresAt](const auto &res) {
                    if (res.empty())
                    {
                        if (self && self->ui)
                        {
                            self->ui->displayError(
                                "No user associated with token");
                        }
                        return;
                    }
                    const auto &user = res.front();
                    TwitchAccountData{
                        .username = user.login,
                        .userID = user.id,
                        .clientID = DEVICE_AUTH_CLIENT_ID,
                        .oauthToken = accessToken,
                        .ty = TwitchAccount::Type::DeviceAuth,
                        .refreshToken = refreshToken,
                        .expiresAt = expiresAt,
                    }
                        .save();
                    getApp()->getAccounts()->twitch.reloadUsers();
                    getApp()->getAccounts()->twitch.currentUsername =
                        user.login;
                    getSettings()->requestSave();

                    if (self)
                    {
                        if (self->ui)
                        {
                            self->ui->window()->close();
                        }
                        self->deleteLater();
                    }
                },
                [ui{this->ui}]() {
                    if (ui)
                    {
                        ui->displayError(
                            u"Failed to fetch authenticated user"_s);
                    }
                });
        })
        .onError([this](const auto &res) {
            auto json = res.parseJson();
            auto message = json["message"_L1].toString(u"(no message)"_s);
            if (message != u"authorization_pending"_s && this->ui)
            {
                this->ui->displayError(res.formatError() + u" - "_s + message);
            }
        })
        .execute();
}

}  // namespace

namespace chatterino {

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

        if (logInWithImplicitGrantCredentials(this, userID, username, clientID,
                                              oauthToken))
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

                logInWithImplicitGrantCredentials(this, userID, username,
                                                  clientID, oauthToken);
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
    this->setMinimumWidth(400);
    this->setWindowFlags(
        (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    this->setWindowTitle("Add new account");

    this->setLayout(&this->ui_.mainLayout);
    this->ui_.mainLayout.addWidget(&this->ui_.tabWidget);

    this->ui_.tabWidget.addTab(new DeviceLoginWidget, "Device");
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
