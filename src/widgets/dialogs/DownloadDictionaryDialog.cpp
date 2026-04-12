// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/DownloadDictionaryDialog.hpp"

#include "Application.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "controllers/spellcheck/SpellChecker.hpp"
#include "singletons/Paths.hpp"
#include "util/Expected.hpp"
#include "util/FilesystemHelpers.hpp"

#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QWidget>

namespace {

using namespace Qt::Literals;
using namespace chatterino;

const QString DICTIONARIES_BASE_URL = u"http://localhost:8080"_s;

}  // namespace

namespace chatterino {

DownloadDictionaryDialog::DownloadDictionaryDialog(QWidget *parent)
    : BasePopup(
          {
              BaseWindow::EnableCustomFrame,
              BaseWindow::DisableLayoutSave,
          },
          parent)
{
    this->setWindowTitle("Download Dictionary — Chatterino");
    this->setAttribute(Qt::WA_DeleteOnClose);

    auto *layout = new QFormLayout(this->getLayoutContainer());
    layout->setSpacing(5);

    layout->addRow(new QLabel("Select a dictionary to download."));

    this->ui.dictionaries = new QComboBox;
    this->ui.dictionaries->setEnabled(false);
    layout->addRow("Dictionary:", this->ui.dictionaries);

    this->ui.status = new QLabel("Loading dictionaries…");
    layout->addRow(this->ui.status);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::rejected, this,
                     &DownloadDictionaryDialog::close);

    auto *showLicense = new QPushButton("Show License");
    QObject::connect(showLicense, &QPushButton::clicked, this,
                     &DownloadDictionaryDialog::showLicense);
    layout->addRow(showLicense);

    this->ui.downloadButton =
        buttons->addButton("Download", QDialogButtonBox::AcceptRole);
    this->ui.downloadButton->setEnabled(false);
    QObject::connect(buttons, &QDialogButtonBox::accepted, this,
                     &DownloadDictionaryDialog::doDownload);
    // Add stretch
    layout->addItem(new QSpacerItem(0, 10, QSizePolicy::Ignored,
                                    QSizePolicy::MinimumExpanding));
    layout->addRow(buttons);

    NetworkRequest(QString(DICTIONARIES_BASE_URL % u"/index.json"))
        .caller(this)
        .onError([this](const NetworkResult &result) {
            this->ui.status->setText("Failed to load dictionaries: " %
                                     result.formatError());
        })
        .onSuccess([this](const NetworkResult &result) {
            this->ui.downloadButton->setEnabled(true);
            this->ui.status->hide();

            QComboBox *dicts = this->ui.dictionaries;
            dicts->setEnabled(true);

            auto json = result.parseJsonArray();
            for (const auto val : json)
            {
                auto parsed = RemoteDictionary::fromJson(val.toObject());
                if (!parsed)
                {
                    continue;
                }
                this->dictionaries.emplace_back(*std::move(parsed));
            }
            std::ranges::sort(this->dictionaries,
                              [](const auto &a, const auto &b) {
                                  return a.prettyName < b.prettyName;
                              });

            for (const auto &item : this->dictionaries)
            {
                dicts->addItem(item.prettyName);
            }
        })
        .execute();
}

void DownloadDictionaryDialog::doDownload()
{
    const auto *selected = this->selected();
    if (!selected)
    {
        return;
    }
    auto rootDir = qStringToStdPath(getApp()->getPaths().dictionariesDirectory);
    auto basePath = rootDir / qStringToStdPath(selected->bcp47);
    auto dicPath = basePath;
    auto affPath = basePath;
    auto licPath = basePath;
    dicPath += ".dic";
    affPath += ".aff";
    licPath += ".lic";

    if (std::filesystem::exists(dicPath) || std::filesystem::exists(affPath) ||
        std::filesystem::exists(licPath))
    {
        auto res = QMessageBox::warning(
            this, "Dictionary Exists",
            "The dictionary already exists. Do you want to overwrite it?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (res != QMessageBox::Yes)
        {
            return;
        }
    }

    struct Results {
        QString errors;
        size_t nDone = 0;
    };
    auto results = std::make_shared<Results>();
    auto pushAndCheck = [this, results](const QString &err) {
        if (!err.isEmpty())
        {
            if (!results->errors.isEmpty())
            {
                results->errors.append('\n');
            }
            results->errors.append(err);
        }
        ++results->nDone;

        // We're the last, close the dialog
        if (results->nDone == 3)
        {
            if (results->errors.isEmpty())
            {
                getApp()->getSpellChecker()->dictionariesUpdated.invoke();
                this->close();
            }
            else
            {
                QMessageBox::warning(this, "Dictionary Download Failed",
                                     results->errors);
            }
        }
    };
    auto saveTo = [&](const std::filesystem::path &path) {
        return [pushAndCheck,
                path = stdPathToQString(path)](const NetworkResult &res) {
            QFile f(path);
            if (!f.open(QFile::WriteOnly))
            {
                pushAndCheck("Failed to open " % path % ": " % f.errorString());
                return;
            }
            f.write(res.getData());
            pushAndCheck({});
        };
    };

    NetworkRequest(QString(DICTIONARIES_BASE_URL % '/' % selected->dic))
        .caller(this)
        .onError([pushAndCheck](const NetworkResult &res) {
            pushAndCheck("Failed to download *.dic file: " % res.formatError());
        })
        .onSuccess(saveTo(dicPath))
        .execute();

    NetworkRequest(QString(DICTIONARIES_BASE_URL % '/' % selected->aff))
        .caller(this)
        .onError([pushAndCheck](const NetworkResult &res) {
            pushAndCheck("Failed to download *.aff file: " % res.formatError());
        })
        .onSuccess(saveTo(affPath))
        .execute();

    if (selected->licence.isEmpty())
    {
        // Already done with the license.
        pushAndCheck({});
    }
    else
    {
        NetworkRequest(QString(DICTIONARIES_BASE_URL % '/' % selected->licence))
            .caller(this)
            .onError([pushAndCheck](const NetworkResult &res) {
                pushAndCheck("Failed to download *.lic file: " %
                             res.formatError());
            })
            .onSuccess(saveTo(licPath))
            .execute();
    }
}

void DownloadDictionaryDialog::showLicense()
{
    const auto *selected = this->selected();
    if (selected)
    {
        selected->showLicense(this);
    }
}

const DownloadDictionaryDialog::RemoteDictionary *
    DownloadDictionaryDialog::selected() const
{
    auto idx = this->ui.dictionaries->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(this->dictionaries.size()))
    {
        return nullptr;
    }
    return &this->dictionaries[static_cast<size_t>(idx)];
}

std::optional<DownloadDictionaryDialog::RemoteDictionary>
    DownloadDictionaryDialog::RemoteDictionary::fromJson(const QJsonObject &obj)
{
    std::optional<RemoteDictionary> ret{{
        .prettyName = {},
        .bcp47 = obj["bcp47"_L1].toString(),
        .spdxLicenseID = obj["spdx_license_id"_L1].toString(),
        .dic = obj["dic"_L1].toString(),
        .aff = obj["aff"_L1].toString(),
        .licence = obj["license"_L1].toString(),
    }};
    if (ret->bcp47.isEmpty() || ret->spdxLicenseID.isEmpty() ||
        ret->dic.isEmpty() || ret->aff.isEmpty())
    {
        ret.reset();
    }
    else
    {
        ret->prettyName = spellcheck::prettyLossyBcp47Description(ret->bcp47);
    }
    return ret;
}

void DownloadDictionaryDialog::RemoteDictionary::showLicense(
    QWidget *parent) const
{
    if (!this->licence.isEmpty())
    {
        QDesktopServices::openUrl(
            QString(DICTIONARIES_BASE_URL % '/' % this->licence));
        return;
    }
    QMessageBox::information(parent, "License",
                             "This dictionary is licensed under " %
                                 this->spdxLicenseID %
                                 ". There is no full license text available.");
}

}  // namespace chatterino
