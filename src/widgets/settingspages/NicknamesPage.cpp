#include "widgets/settingspages/NicknamesPage.hpp"

#include "controllers/nicknames/Nickname.hpp"
#include "controllers/nicknames/NicknamesModel.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QFileDialog>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTableView>
namespace chatterino {

NicknamesPage::NicknamesPage()
{
    LayoutCreator<NicknamesPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>(
        "Nicknames do not work with features such as user highlights and "
        "filters."
        "\nWith those features you will still need to use the user's original "
        "name.");
    EditableModelView *view =
        layout
            .emplace<EditableModelView>(
                (new NicknamesModel(nullptr))
                    ->initialized(&getSettings()->nicknames))
            .getElement();
    this->view_ = view;

    view->setTitles({"Username", "Nickname", "Enable regex", "Case-sensitive"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Fixed);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);

    // We can safely ignore this signal connection since we own the view
    std::ignore = view->addButtonPressed.connect([] {
        getSettings()->nicknames.append(
            Nickname{"Username", "Nickname", false, false});
    });

    // Add Import and Export buttons to the EditableModelView
    auto *importButton = new QPushButton("Import");
    auto *exportButton = new QPushButton("Export");

    QObject::connect(importButton, &QPushButton::clicked, this,
                     &NicknamesPage::importNicknames);
    QObject::connect(exportButton, &QPushButton::clicked, this,
                     &NicknamesPage::exportNicknames);

    view->addCustomButton(importButton);
    view->addCustomButton(exportButton);

    QTimer::singleShot(1, [view] {
        view->getTableView()->resizeColumnsToContents();
        view->getTableView()->setColumnWidth(0, 200);
    });
}

bool NicknamesPage::filterElements(const QString &query)
{
    std::array fields{0, 1};

    return this->view_->filterSearchResults(query, fields);
}

void NicknamesPage::importNicknames()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Import Nicknames"), "", tr("JSON Files (*.json)"));

    if (filePath.isEmpty())
    {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, tr("Error"),
                              tr("Could not open file for reading."));
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isArray())
    {
        QMessageBox::critical(
            this, tr("Error"),
            tr("Invalid JSON format. Expected an array of nicknames."));
        return;
    }

    QJsonArray array = doc.array();
    for (const QJsonValueRef &value : array)
    {
        if (!value.isObject())
        {
            continue;
        }

        QJsonObject obj = value.toObject();
        QString username = obj["username"].toString();
        QString nickname = obj["nickname"].toString();
        bool regex = obj["regex"].toBool(false);
        bool caseSensitive = obj["caseSensitive"].toBool(false);

        if (!username.isEmpty() && !nickname.isEmpty())
        {
            getSettings()->nicknames.append(
                Nickname{username, nickname, regex, caseSensitive});
        }
    }
}

void NicknamesPage::exportNicknames()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Export Nicknames"), "", tr("JSON Files (*.json)"));

    if (filePath.isEmpty())
    {
        return;
    }

    if (!filePath.endsWith(".json", Qt::CaseInsensitive))
    {
        filePath += ".json";
    }

    QJsonArray array;
    const auto &nicknames = getSettings()->nicknames.raw();

    for (const auto &nickname : nicknames)
    {
        QJsonObject obj;
        obj["username"] = nickname.name();
        obj["nickname"] = nickname.replace();
        obj["regex"] = nickname.isRegex();
        obj["caseSensitive"] = nickname.isCaseSensitive();
        array.append(obj);
    }

    QJsonDocument doc(array);
    QByteArray data = doc.toJson(QJsonDocument::Indented);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(this, tr("Error"),
                              tr("Could not open file for writing."));
        return;
    }

    if (file.write(data) == -1)
    {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to write to file."));
    }
}

}  // namespace chatterino
