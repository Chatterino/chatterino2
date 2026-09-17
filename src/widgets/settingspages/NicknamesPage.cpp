// SPDX-FileCopyrightText: 2021 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/settingspages/NicknamesPage.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/nicknames/Nickname.hpp"
#include "controllers/nicknames/NicknamesModel.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"
#include "widgets/helper/SvgWidget.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>

#include <functional>
#include <utility>
#include <vector>

namespace {

using namespace chatterino;

class NicknameDialog : public QDialog
{
public:
    NicknameDialog(QWidget *parent,
                   std::function<void(const NicknameEntry &)> save,
                   const std::optional<NicknameEntry> &existing = {},
                   bool lookupByUserID = false)
        : QDialog(parent)
        , save_(std::move(save))
        , existing_(existing)
        , type_(new QComboBox(this))
        , form_(new QFormLayout)
        , accountLookupLabel_(new QWidget(this))
        , lookupType_(new QComboBox(this))
        , accountLookupInfo_(new SvgWidget(this))
        , userLabel_(new QLabel(this))
        , user_(new QStackedWidget(this))
        , twitchUsername_(new QLineEdit(this))
        , userID_(new QLineEdit(this))
        , username_(new QLineEdit(this))
        , pattern_(new QLineEdit(this))
        , nickname_(new QLineEdit(this))
        , caseSensitive_(new QCheckBox("Case-sensitive", this))
        , error_(new QLabel(this))
        , buttons_(new QDialogButtonBox(
              QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this))
    {
        this->setWindowTitle(existing ? "Edit nickname" : "Add nickname");
        this->setMinimumWidth(360);
        auto *layout = new QVBoxLayout(this);
        this->type_->addItem(
            "Twitch account",
            static_cast<int>(NicknameEntryType::TwitchAccount));
        this->type_->addItem("Username (legacy)",
                             static_cast<int>(NicknameEntryType::Username));
        this->type_->addItem("Regex",
                             static_cast<int>(NicknameEntryType::Regex));
        this->form_->addRow("Match on:", this->type_);

        this->lookupType_->addItems({"Username", "User ID"});
        this->accountLookupInfo_->load(QStringLiteral(":/settings/hint.svg"));
        this->accountLookupInfo_->setToolTip(
            "This only controls how the account is found. The saved entry "
            "always uses its User ID.");
        const auto infoHeight = this->lookupType_->sizeHint().height();
        this->accountLookupInfo_->setFixedSize(infoHeight / 2, infoHeight);
        auto *accountLookupLabelLayout =
            new QHBoxLayout(this->accountLookupLabel_);
        accountLookupLabelLayout->setContentsMargins(0, 0, 0, 0);
        accountLookupLabelLayout->addWidget(
            new QLabel("Find account by:", this->accountLookupLabel_));
        accountLookupLabelLayout->addWidget(this->accountLookupInfo_);
        this->user_->addWidget(this->twitchUsername_);
        this->user_->addWidget(this->userID_);
        this->user_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        this->user_->setFixedHeight(this->twitchUsername_->sizeHint().height());
        this->form_->addRow(this->accountLookupLabel_, this->lookupType_);
        this->form_->addRow(this->userLabel_, this->user_);
        this->form_->addRow("Username:", this->username_);
        this->form_->addRow("Pattern:", this->pattern_);
        this->form_->addRow("Nickname:", this->nickname_);
        this->form_->addRow(this->caseSensitive_);
        layout->addLayout(this->form_);

        this->error_->setWordWrap(true);
        this->error_->setVisible(false);
        layout->addWidget(this->error_);
        layout->addWidget(this->buttons_);

        QObject::connect(this->buttons_, &QDialogButtonBox::accepted, this,
                         [this] {
                             this->save();
                         });
        QObject::connect(this->buttons_, &QDialogButtonBox::rejected, this,
                         &QDialog::reject);
        QObject::connect(this->lookupType_, &QComboBox::currentIndexChanged,
                         this, [this](int index) {
                             this->user_->setCurrentIndex(index);
                             this->userLabel_->setText(index == 0 ? "Username:"
                                                                  : "User ID:");
                         });
        QObject::connect(this->type_, &QComboBox::currentIndexChanged, this,
                         [this] {
                             this->updateTypeFields();
                         });

        if (existing)
        {
            this->type_->setCurrentIndex(
                this->type_->findData(static_cast<int>(existing->type)));
            if (existing->type == NicknameEntryType::TwitchAccount)
            {
                this->twitchUsername_->setText(existing->username);
                this->userID_->setText(existing->userID);
            }
            else if (existing->type == NicknameEntryType::Username)
            {
                this->username_->setText(existing->username);
            }
            else
            {
                this->pattern_->setText(existing->username);
            }
            this->nickname_->setText(existing->nickname);
            this->caseSensitive_->setChecked(existing->caseSensitive);
        }
        this->lookupType_->setCurrentIndex(lookupByUserID ? 1 : 0);
        this->user_->setCurrentIndex(this->lookupType_->currentIndex());
        this->userLabel_->setText(lookupByUserID ? "User ID:" : "Username:");
        this->updateTypeFields();
    }

private:
    void save()
    {
        const auto type = this->currentType();
        const auto nickname = type == NicknameEntryType::Regex
                                  ? this->nickname_->text()
                                  : this->nickname_->text().trimmed();
        if (type != NicknameEntryType::Regex && nickname.isEmpty())
        {
            this->showError("Enter a nickname.");
            return;
        }

        if (type != NicknameEntryType::TwitchAccount)
        {
            const auto value = type == NicknameEntryType::Username
                                   ? this->username_->text().trimmed()
                                   : this->pattern_->text().trimmed();
            if (value.isEmpty())
            {
                this->showError(type == NicknameEntryType::Username
                                    ? "Enter a username."
                                    : "Enter a pattern.");
                return;
            }
            this->save_({
                .type = type,
                .username = value,
                .nickname = nickname,
                .caseSensitive = this->caseSensitive_->isChecked(),
            });
            this->accept();
            return;
        }

        const bool lookupByUserID = this->lookupType_->currentIndex() == 1;
        auto user = lookupByUserID ? this->userID_->text().trimmed()
                                   : this->twitchUsername_->text().trimmed();
        if (!lookupByUserID && user.startsWith('@'))
        {
            user.remove(0, 1);
        }
        if (user.isEmpty())
        {
            this->showError(lookupByUserID ? "Enter a User ID."
                                           : "Enter a username.");
            return;
        }
        if (this->existing_ &&
            this->existing_->type == NicknameEntryType::TwitchAccount &&
            ((lookupByUserID && user == this->existing_->userID) ||
             (!lookupByUserID && user.compare(this->existing_->username,
                                              Qt::CaseInsensitive) == 0)))
        {
            auto replacement = *this->existing_;
            replacement.nickname = nickname;
            this->save_(replacement);
            this->accept();
            return;
        }
        if (getApp()->getAccounts()->twitch.getCurrent()->isAnon())
        {
            this->showError(
                "You need to be logged in to look up a Twitch account.");
            return;
        }

        this->setLookupPending(true);
        this->error_->setVisible(false);
        const QPointer<NicknameDialog> self(this);
        const auto success = [self, nickname](const HelixUser &resolved) {
            if (self == nullptr)
            {
                return;
            }
            self->save_({
                .type = NicknameEntryType::TwitchAccount,
                .username = resolved.login,
                .userID = resolved.id,
                .nickname = nickname,
            });
            self->accept();
        };
        const auto failure = [self] {
            if (self != nullptr)
            {
                self->setLookupPending(false);
                self->showError("No Twitch account was found.");
            }
        };
        if (lookupByUserID)
        {
            getHelix()->getUserById(user, success, failure);
        }
        else
        {
            getHelix()->getUserByName(user, success, failure);
        }
    }

    void showError(const QString &message)
    {
        this->error_->setText(message);
        this->error_->setVisible(true);
    }

    void setLookupPending(bool pending)
    {
        const bool enabled = !pending;
        this->type_->setEnabled(enabled);
        this->lookupType_->setEnabled(enabled);
        this->user_->setEnabled(enabled);
        this->username_->setEnabled(enabled);
        this->pattern_->setEnabled(enabled);
        this->nickname_->setEnabled(enabled);
        this->caseSensitive_->setEnabled(enabled);
        this->buttons_->button(QDialogButtonBox::Ok)->setEnabled(enabled);
    }

    NicknameEntryType currentType() const
    {
        return static_cast<NicknameEntryType>(
            this->type_->currentData().toInt());
    }

    void updateTypeFields()
    {
        const auto type = this->currentType();
        const bool twitchUser = type == NicknameEntryType::TwitchAccount;
        this->form_->setRowVisible(this->lookupType_, twitchUser);
        this->form_->setRowVisible(this->user_, twitchUser);
        this->form_->setRowVisible(this->username_,
                                   type == NicknameEntryType::Username);
        this->form_->setRowVisible(this->pattern_,
                                   type == NicknameEntryType::Regex);
        this->form_->setRowVisible(this->caseSensitive_, !twitchUser);
        this->adjustSize();
    }

    std::function<void(const NicknameEntry &)> save_;
    std::optional<NicknameEntry> existing_;
    QComboBox *type_{};
    QFormLayout *form_{};
    QWidget *accountLookupLabel_{};
    QComboBox *lookupType_{};
    SvgWidget *accountLookupInfo_{};
    QLabel *userLabel_{};
    QStackedWidget *user_{};
    QLineEdit *twitchUsername_{};
    QLineEdit *userID_{};
    QLineEdit *username_{};
    QLineEdit *pattern_{};
    QLineEdit *nickname_{};
    QCheckBox *caseSensitive_{};
    QLabel *error_{};
    QDialogButtonBox *buttons_{};
};

Nickname toLegacyNickname(const NicknameEntry &entry)
{
    return {entry.username, entry.nickname,
            entry.type == NicknameEntryType::Regex, entry.caseSensitive};
}

void addNickname(const NicknameEntry &entry, NicknamesModel *model)
{
    if (entry.type == NicknameEntryType::TwitchAccount)
    {
        model->setAccountNickname(entry.userID, entry.username, entry.nickname);
        return;
    }
    getSettings()->nicknames.append(toLegacyNickname(entry));
}

void replaceNickname(const QPersistentModelIndex &index, NicknamesModel *model,
                     const NicknameEntry &replacement)
{
    if (!index.isValid())
    {
        return;
    }

    const auto previous = model->entryAt(index.row());
    if (!previous)
    {
        return;
    }

    if (replacement.type == NicknameEntryType::TwitchAccount)
    {
        model->setAccountNickname(replacement.userID, replacement.username,
                                  replacement.nickname);
        if (previous->type != NicknameEntryType::TwitchAccount)
        {
            getSettings()->nicknames.removeAt(index.row());
        }
        else if (previous->userID != replacement.userID)
        {
            model->setAccountNickname(previous->userID, previous->username, {});
        }
        return;
    }

    const auto nickname = toLegacyNickname(replacement);
    if (previous->type == NicknameEntryType::TwitchAccount)
    {
        getSettings()->nicknames.append(nickname);
        model->setAccountNickname(previous->userID, previous->username, {});
    }
    else
    {
        const auto row = index.row();
        getSettings()->nicknames.removeAt(row);
        getSettings()->nicknames.insert(nickname, row);
    }
}

void checkNicknameDuplicates(EditableModelView *view, NicknamesModel *model,
                             QLabel *duplicateWarning)
{
    struct UsernameEntry {
        QString username;
        int row;
        bool caseSensitive;
        bool twitchUser;
    };
    std::vector<UsernameEntry> usernames;
    std::vector<bool> duplicateUsernames(model->rowCount(QModelIndex{}), false);
    const auto warningColor =
        view->getTableView()->palette().color(QPalette::Link);
    const auto disabledColor =
        QApplication::palette().color(QPalette::Disabled, QPalette::Text);

    for (int row = 0; row < model->rowCount(QModelIndex{}); ++row)
    {
        const auto entry = model->entryAt(row);
        if (!entry)
        {
            continue;
        }
        model->getItem(row, 1)->setData(
            entry->type == NicknameEntryType::TwitchAccount ? disabledColor
                                                            : QVariant{},
            Qt::ForegroundRole);
        model->getItem(row, 2)->setData(
            entry->type == NicknameEntryType::TwitchAccount ? disabledColor
                                                            : QVariant{},
            Qt::ForegroundRole);

        if (entry->type == NicknameEntryType::TwitchAccount)
        {
            if (!entry->username.isEmpty())
            {
                usernames.push_back({
                    .username = entry->username,
                    .row = row,
                    .caseSensitive = false,
                    .twitchUser = true,
                });
            }
        }
        else if (entry->type == NicknameEntryType::Username)
        {
            const auto username = entry->username.trimmed();
            if (username.isEmpty())
            {
                continue;
            }
            usernames.push_back({
                .username = username,
                .row = row,
                .caseSensitive = entry->caseSensitive,
                .twitchUser = false,
            });
        }
    }

    bool foundDuplicate = false;
    for (size_t i = 0; i < usernames.size(); ++i)
    {
        for (size_t j = i + 1; j < usernames.size(); ++j)
        {
            if (usernames[i].twitchUser && usernames[j].twitchUser)
            {
                continue;
            }
            const auto sensitivity =
                usernames[i].caseSensitive && usernames[j].caseSensitive
                    ? Qt::CaseSensitive
                    : Qt::CaseInsensitive;
            if (usernames[i].username.compare(usernames[j].username,
                                              sensitivity) == 0)
            {
                duplicateUsernames[usernames[i].row] = true;
                duplicateUsernames[usernames[j].row] = true;
            }
        }
    }
    for (size_t row = 0; row < duplicateUsernames.size(); ++row)
    {
        if (duplicateUsernames[row])
        {
            foundDuplicate = true;
            model->getItem(static_cast<int>(row), 1)
                ->setData(warningColor, Qt::ForegroundRole);
        }
    }

    duplicateWarning->setVisible(foundDuplicate);
    view->getTableView()->viewport()->update();
}

}  // namespace

namespace chatterino {

NicknamesPage::NicknamesPage()
{
    this->rememberLegacyNicknames();
    this->managedConnections_.managedConnect(
        getSettings()->nicknames.itemInserted, [this](const auto &) {
            this->legacyNicknamesChanged_ = true;
        });
    this->managedConnections_.managedConnect(
        getSettings()->nicknames.itemRemoved, [this](const auto &) {
            this->legacyNicknamesChanged_ = true;
        });

    LayoutCreator<NicknamesPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>(
        "Nicknames do not work with features such as user highlights and "
        "filters."
        "\nWith those features you will still need to use the user's original "
        "name.");
    layout.emplace<QLabel>(
        "Twitch account entries match by User ID. Legacy username and regex "
        "entries match by name."
        "\nTwitch account entries cannot be reordered.");

    auto *model = new NicknamesModel(getApp()->getUserData(), nullptr,
                                     [this](const QString &userID) {
                                         this->rememberAccountNickname(userID);
                                     });
    model->initialize(&getSettings()->nicknames);
    auto *view = layout.emplace<EditableModelView>(model).getElement();
    this->view_ = view;
    view->setTitles({"Match on", "Username / pattern", "User ID", "Nickname",
                     "Case-sensitive"});
    view->getTableView()->setDragDropMode(QAbstractItemView::NoDragDrop);
    auto *header = view->getTableView()->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    QObject::connect(model, &QAbstractItemModel::rowsInserted, view,
                     [table = view->getTableView()] {
                         // ResizeToContents can retain widths calculated for
                         // the empty table, so recalculate the content-sized
                         // columns after rows are inserted.
                         QTimer::singleShot(0, table, [table] {
                             table->resizeColumnToContents(0);
                             table->resizeColumnToContents(2);
                             table->resizeColumnToContents(4);
                         });
                     });
    view->addRegexHelpLink();

    auto *duplicateWarning =
        layout
            .emplace<QLabel>("There are overlapping nickname entries. Only "
                             "the first matching entry will be used.")
            .getElement();
    auto warningPalette = duplicateWarning->palette();
    warningPalette.setColor(
        QPalette::WindowText,
        view->getTableView()->palette().color(QPalette::Link));
    duplicateWarning->setPalette(warningPalette);
    duplicateWarning->setWordWrap(true);

    const auto checkDuplicates = [view, model, duplicateWarning] {
        checkNicknameDuplicates(view, model, duplicateWarning);
    };
    QObject::connect(model, &QAbstractItemModel::rowsInserted, this,
                     checkDuplicates);
    QObject::connect(model, &QAbstractItemModel::rowsRemoved, this,
                     checkDuplicates);
    QObject::connect(model, &QAbstractItemModel::dataChanged, this,
                     checkDuplicates);
    checkDuplicates();

    std::ignore = view->addButtonPressed.connect([this, model] {
        auto *dialog =
            new NicknameDialog(this, [model](const NicknameEntry &entry) {
                addNickname(entry, model);
            });
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    });
    QObject::connect(
        view->getTableView(), &QTableView::doubleClicked, this,
        [this, model](const QModelIndex &index) {
            const auto existing = model->entryAt(index.row());
            if (!existing)
            {
                return;
            }
            const bool accountColumn =
                existing->type == NicknameEntryType::TwitchAccount &&
                (index.column() == 1 || index.column() == 2);
            if (index.column() != 0 && !accountColumn)
            {
                return;
            }
            const QPersistentModelIndex persistentIndex =
                model->index(index.row(), 0);
            auto *dialog = new NicknameDialog(
                this,
                [persistentIndex, model](const NicknameEntry &replacement) {
                    replaceNickname(persistentIndex, model, replacement);
                },
                existing, index.column() == 2);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->open();
        });
}

bool NicknamesPage::filterElements(const QString &query)
{
    std::array fields{0, 1, 2, 3};
    return this->view_->filterSearchResults(query, fields);
}

void NicknamesPage::onShow()
{
    this->originalAccountNicknames_.clear();
    this->rememberLegacyNicknames();
}

void NicknamesPage::onSettingsDialogAccepted()
{
    this->originalAccountNicknames_.clear();
    this->rememberLegacyNicknames();
}

void NicknamesPage::onSettingsDialogRejected()
{
    auto originals = std::exchange(this->originalAccountNicknames_, {});
    for (auto it = originals.cbegin(); it != originals.cend(); ++it)
    {
        if (*it && !(*it)->nickname.isEmpty())
        {
            getApp()->getUserData()->setUserNickname(
                it.key(), (*it)->lastSeenUsername, (*it)->nickname);
        }
        else
        {
            getApp()->getUserData()->setUserNickname(it.key(), {}, {});
        }
    }
    this->restoreLegacyNicknames();
}

void NicknamesPage::rememberAccountNickname(const QString &userID)
{
    if (!this->originalAccountNicknames_.contains(userID))
    {
        this->originalAccountNicknames_.insert(
            userID, getApp()->getUserData()->getUser(userID));
    }
}

void NicknamesPage::rememberLegacyNicknames()
{
    this->originalLegacyNicknames_ = getSettings()->nicknames.raw();
    this->legacyNicknamesChanged_ = false;
}

void NicknamesPage::restoreLegacyNicknames()
{
    if (!this->legacyNicknamesChanged_)
    {
        return;
    }

    auto &nicknames = getSettings()->nicknames;
    while (!nicknames.raw().empty())
    {
        nicknames.removeAt(static_cast<int>(nicknames.raw().size()) - 1);
    }
    for (const auto &nickname : this->originalLegacyNicknames_)
    {
        nicknames.append(nickname);
    }
    this->legacyNicknamesChanged_ = false;
}

}  // namespace chatterino
