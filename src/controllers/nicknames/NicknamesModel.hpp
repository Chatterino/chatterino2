// SPDX-FileCopyrightText: 2021 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/SignalVectorModel.hpp"
#include "controllers/nicknames/Nickname.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QObject>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chatterino {

class IUserDataController;
struct UserData;

enum class NicknameEntryType : std::uint8_t {
    TwitchAccount,
    Username,
    Regex,
};

struct NicknameEntry {
    NicknameEntryType type;
    QString username;
    QString userID;
    QString nickname;
    bool caseSensitive{};
};

class NicknamesModel : public SignalVectorModel<Nickname>
{
public:
    NicknamesModel(
        IUserDataController *userData, QObject *parent,
        std::function<void(const QString &)> accountNicknameChanging = {});
    std::optional<NicknameEntry> entryAt(int row) const;
    void setAccountNickname(const QString &userID, const QString &username,
                            const QString &nickname);
    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent,
                  int destinationChild) override;

protected:
    // turn a vector item into a model row
    Nickname getItemFromRow(std::vector<QStandardItem *> &row,
                            const Nickname &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const Nickname &item,
                        std::vector<QStandardItem *> &row) override;

    void afterInit() override;
    int beforeInsert(const Nickname &item, std::vector<QStandardItem *> &row,
                     int proposedIndex) override;
    void customRowSetData(const std::vector<QStandardItem *> &row, int column,
                          const QVariant &value, int role,
                          int rowIndex) override;

private:
    void refreshAccountRows();
    int legacyRowCount() const;
    void updateAccountRow(int row, const QString &userID, const UserData &data);

    IUserDataController *userData_{};
    std::function<void(const QString &)> accountNicknameChanging_;
    pajlada::Signals::SignalHolder signalHolder_;
    std::vector<QString> accountUserIDs_;
};

}  // namespace chatterino
