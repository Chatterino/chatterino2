// SPDX-FileCopyrightText: 2021 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/nicknames/NicknamesModel.hpp"

#include "controllers/nicknames/Nickname.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "util/StandardItemHelper.hpp"

#include <QApplication>
#include <QPalette>

#include <unordered_set>

namespace chatterino {

namespace {

QString typeName(NicknameEntryType type)
{
    switch (type)
    {
        case NicknameEntryType::TwitchAccount:
            return "Twitch account";
        case NicknameEntryType::Username:
            return "Username (legacy)";
        case NicknameEntryType::Regex:
            return "Regex";
    }
    return {};
}

void setDisabledText(QStandardItem *item)
{
    item->setData(
        QApplication::palette().color(QPalette::Disabled, QPalette::Text),
        Qt::ForegroundRole);
}

}  // namespace

NicknamesModel::NicknamesModel(
    IUserDataController *userData, QObject *parent,
    std::function<void(const QString &)> accountNicknameChanging)
    : SignalVectorModel<Nickname>(5, parent)
    , userData_(userData)
    , accountNicknameChanging_(std::move(accountNicknameChanging))
{
    this->signalHolder_.managedConnect(userData->userDataUpdated(), [this] {
        this->refreshAccountRows();
    });
}

void NicknamesModel::setAccountNickname(const QString &userID,
                                        const QString &username,
                                        const QString &nickname)
{
    if (this->accountNicknameChanging_)
    {
        this->accountNicknameChanging_(userID);
    }
    this->userData_->setUserNickname(userID, username, nickname);
}

std::optional<NicknameEntry> NicknamesModel::entryAt(int row) const
{
    if (row < 0 || row >= this->rowCount({}))
    {
        return std::nullopt;
    }

    const auto legacyRows = this->legacyRowCount();
    if (row < legacyRows)
    {
        const auto &original = this->rows()[static_cast<size_t>(row)].original;
        if (!original)
        {
            return std::nullopt;
        }
        return NicknameEntry{
            .type = original->isRegex() ? NicknameEntryType::Regex
                                        : NicknameEntryType::Username,
            .username =
                this->data(this->index(row, 1), Qt::DisplayRole).toString(),
            .nickname =
                this->data(this->index(row, 3), Qt::DisplayRole).toString(),
            .caseSensitive =
                this->data(this->index(row, 4), Qt::CheckStateRole).toBool(),
        };
    }

    const auto &userID =
        this->accountUserIDs_[static_cast<size_t>(row - legacyRows)];
    const auto data = this->userData_->getUser(userID);
    if (!data || data->nickname.isEmpty())
    {
        return std::nullopt;
    }
    return NicknameEntry{
        .type = NicknameEntryType::TwitchAccount,
        .username = data->lastSeenUsername,
        .userID = userID,
        .nickname = data->nickname,
    };
}

bool NicknamesModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count != 1 || row < 0 || row >= this->rowCount(parent))
    {
        return false;
    }

    if (row < this->legacyRowCount())
    {
        return SignalVectorModel<Nickname>::removeRows(row, count, parent);
    }

    const auto entry = this->entryAt(row);
    if (!entry || entry->type != NicknameEntryType::TwitchAccount)
    {
        return false;
    }
    this->setAccountNickname(entry->userID, entry->username, {});
    return true;
}

bool NicknamesModel::moveRows(const QModelIndex &sourceParent, int sourceRow,
                              int count, const QModelIndex &destinationParent,
                              int destinationChild)
{
    const auto legacyRows = this->legacyRowCount();
    if (sourceRow < 0 || sourceRow >= legacyRows || destinationChild < 0 ||
        destinationChild >= legacyRows)
    {
        return false;
    }
    return SignalVectorModel<Nickname>::moveRows(
        sourceParent, sourceRow, count, destinationParent, destinationChild);
}

// turn a vector item into a model row
Nickname NicknamesModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                        const Nickname &original)
{
    return Nickname{row[1]->data(Qt::DisplayRole).toString().trimmed(),
                    row[3]->data(Qt::DisplayRole).toString(),
                    original.isRegex(),
                    row[4]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void NicknamesModel::getRowFromItem(const Nickname &item,
                                    std::vector<QStandardItem *> &row)
{
    setStringItem(row[0],
                  typeName(item.isRegex() ? NicknameEntryType::Regex
                                          : NicknameEntryType::Username),
                  false);
    setStringItem(row[1], item.name());
    setStringItem(row[2], {}, false);
    setStringItem(row[3], item.replace());
    setBoolItem(row[4], item.isCaseSensitive());
}

void NicknamesModel::afterInit()
{
    this->refreshAccountRows();
}

int NicknamesModel::beforeInsert(const Nickname &item,
                                 std::vector<QStandardItem *> &row,
                                 int proposedIndex)
{
    (void)item;
    (void)row;
    return std::min(proposedIndex, this->legacyRowCount());
}

void NicknamesModel::customRowSetData(const std::vector<QStandardItem *> &row,
                                      int column, const QVariant &value,
                                      int role, int rowIndex)
{
    (void)value;
    if (column != 3 || (role != Qt::EditRole && role != Qt::DisplayRole))
    {
        return;
    }

    const auto accountIndex = rowIndex - this->legacyRowCount();
    if (accountIndex < 0 ||
        static_cast<size_t>(accountIndex) >= this->accountUserIDs_.size())
    {
        return;
    }
    const auto userID =
        this->accountUserIDs_[static_cast<size_t>(accountIndex)];
    this->setAccountNickname(userID, row[1]->data(Qt::DisplayRole).toString(),
                             row[3]->data(Qt::DisplayRole).toString());

    const auto updated = std::ranges::find(this->accountUserIDs_, userID);
    if (updated != this->accountUserIDs_.end())
    {
        const auto updatedRow = this->legacyRowCount() +
                                static_cast<int>(std::distance(
                                    this->accountUserIDs_.begin(), updated));
        const auto updatedIndex = this->index(updatedRow, column);
        Q_EMIT this->dataChanged(updatedIndex, updatedIndex, {role});
    }
}

int NicknamesModel::legacyRowCount() const
{
    const auto account =
        std::ranges::find_if(this->rows(), [](const auto &row) {
            return row.isCustomRow;
        });
    return static_cast<int>(std::distance(this->rows().begin(), account));
}

void NicknamesModel::updateAccountRow(int row, const QString &userID,
                                      const UserData &data)
{
    std::array<QVariant, 5> values{typeName(NicknameEntryType::TwitchAccount),
                                   data.lastSeenUsername,
                                   userID,
                                   data.nickname,
                                   {}};
    for (size_t column = 0; column < values.size(); ++column)
    {
        const auto modelColumn = static_cast<int>(column);
        const auto index = this->index(row, modelColumn);
        auto *item = this->getItem(row, modelColumn);
        if (this->data(index, Qt::DisplayRole) != values.at(column))
        {
            item->setData(values.at(column), Qt::DisplayRole);
            Q_EMIT this->dataChanged(index, index,
                                     {Qt::DisplayRole, Qt::EditRole});
        }
    }
}

void NicknamesModel::refreshAccountRows()
{
    const auto users = this->userData_->getUsers();
    const auto hasNickname = [&users](const QString &userID) {
        const auto user = users.find(userID);
        return user != users.end() && !user->second.nickname.isEmpty();
    };

    for (int index = static_cast<int>(this->accountUserIDs_.size()) - 1;
         index >= 0; --index)
    {
        if (hasNickname(this->accountUserIDs_[static_cast<size_t>(index)]))
        {
            continue;
        }
        const auto row = this->legacyRowCount() + index;
        const auto items = this->rows()[static_cast<size_t>(row)].items;
        this->accountUserIDs_.erase(this->accountUserIDs_.begin() + index);
        this->removeCustomRow(row);
        for (auto *item : items)
        {
            delete item;
        }
    }

    std::unordered_set<QString> existing(this->accountUserIDs_.begin(),
                                         this->accountUserIDs_.end());
    std::vector<QString> added;
    for (const auto &[userID, data] : users)
    {
        if (!data.nickname.isEmpty() && !existing.contains(userID))
        {
            added.push_back(userID);
        }
    }
    std::ranges::sort(
        added, [&users](const QString &left, const QString &right) {
            const auto comparison = users.at(left).lastSeenUsername.compare(
                users.at(right).lastSeenUsername, Qt::CaseInsensitive);
            return comparison == 0 ? left < right : comparison < 0;
        });
    for (const auto &userID : added)
    {
        auto row = this->createRow();
        setStringItem(row[0], typeName(NicknameEntryType::TwitchAccount),
                      false);
        setStringItem(row[1], users.at(userID).lastSeenUsername, false);
        setStringItem(row[2], userID, false);
        setStringItem(row[3], users.at(userID).nickname);
        setStringItem(row[4], {}, false);
        setDisabledText(row[1]);
        setDisabledText(row[2]);
        this->accountUserIDs_.push_back(userID);
        this->insertCustomRow(std::move(row), this->rowCount({}));
    }

    const auto firstAccountRow = this->legacyRowCount();
    for (size_t index = 0; index < this->accountUserIDs_.size(); ++index)
    {
        const auto &userID = this->accountUserIDs_[index];
        this->updateAccountRow(firstAccountRow + static_cast<int>(index),
                               userID, users.at(userID));
    }
}

}  // namespace chatterino
