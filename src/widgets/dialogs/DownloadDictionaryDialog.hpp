// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BasePopup.hpp"

#include <QString>
#include <QVBoxLayout>

#include <optional>

class QComboBox;
class QJsonObject;
class QLabel;
class QPushButton;

namespace chatterino {

class DownloadDictionaryDialog : public BasePopup
{
public:
    DownloadDictionaryDialog(QWidget *parent = nullptr);

private:
    struct RemoteDictionary {
        QString prettyName;
        QString bcp47;
        QString spdxLicenseID;
        QString dic;
        QString aff;
        QString licence;

        static std::optional<RemoteDictionary> fromJson(const QJsonObject &obj);
        void showLicense(QWidget *parent) const;
    };

    void doDownload();
    void showLicense();

    const RemoteDictionary *selected() const;

    struct {
        QComboBox *dictionaries = nullptr;
        QLabel *status = nullptr;
        QPushButton *downloadButton = nullptr;
    } ui;
    std::vector<RemoteDictionary> dictionaries;
};

}  // namespace chatterino
